/* This file contains the main program of the process manager and some related
 * procedures.  When MINIX starts up, the kernel runs for a little while,
 * initializing itself and its tasks, and then it runs PM and VFS.  Both PM
 * and VFS initialize themselves as far as they can. PM asks the kernel for
 * all free memory and starts serving requests.
 *
 * The entry points into this file are:
 *   main:	starts PM running
 *   setreply:	set the reply to be sent to process making an PM system call
 */

#include "pm.h"
#include <minix/keymap.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/ds.h>
#include <minix/type.h>
#include <minix/endpoint.h>
#include <minix/minlib.h>
#include <minix/type.h>
#include <minix/vm.h>
#include <minix/crtso.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <string.h>
#include <machine/archtypes.h>
#include <env.h>
#include "mproc.h"
#include "param.h"
#include "unode.h"

#include "kernel/const.h"
#include "kernel/config.h"
#include "kernel/proc.h"

#if ENABLE_SYSCALL_STATS
EXTERN unsigned long calls_stats[NCALLS];
#endif

FORWARD _PROTOTYPE( void sendreply, (void)				);
FORWARD _PROTOTYPE( int get_nice_value, (int queue)			);
FORWARD _PROTOTYPE( void handle_vfs_reply, (void)			);

#define click_to_round_k(n) \
	((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))

extern int unmap_ok;

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init_fresh, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_signal_manager, (endpoint_t target, int signo) );

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
PUBLIC int main()
{
/* Main routine of the process manager. */
  int result;

  /* SEF local startup. */
  sef_local_startup();

  ulist_init(); /*  Initialize ulist */

  /* This is PM's main loop-  get work and do it, forever and forever. */
  while (TRUE) {
	  int ipc_status;

	  /* Wait for the next message and extract useful information from it. */
	  if (sef_receive_status(ANY, &m_in, &ipc_status) != OK)
		  panic("PM sef_receive_status error");
	  who_e = m_in.m_source;	/* who sent the message */
	  if(pm_isokendpt(who_e, &who_p) != OK)
		  panic("PM got message from invalid endpoint: %d", who_e);
	  call_nr = m_in.m_type;	/* system call number */

	  /* Process slot of caller. Misuse PM's own process slot if the kernel is
	   * calling. This can happen in case of synchronous alarms (CLOCK) or or
	   * event like pending kernel signals (SYSTEM).
	   */
	  mp = &mproc[who_p < 0 ? PM_PROC_NR : who_p];
	  if(who_p >= 0 && mp->mp_endpoint != who_e) {
		  panic("PM endpoint number out of sync with source: %d",
				  			mp->mp_endpoint);
	  }

	/* Drop delayed calls from exiting processes. */
	if (mp->mp_flags & EXITING)
		continue;

    /* Search for user slot in ulist and set it in glo.h variable */
    un = unode_get_always(mp->mp_realuid); 

	/* Check for system notifications first. Special cases. */
	if (is_ipc_notify(ipc_status)) {
		switch(who_p) {
			case CLOCK:
				expire_timers(m_in.NOTIFY_TIMESTAMP);
				result = SUSPEND;	/* don't reply */
				break;
			default :
				result = ENOSYS;
		}

		/* done, send reply and continue */
		if (result != SUSPEND) setreply(who_p, result);
		sendreply();
		continue;
	}

	switch(call_nr)
	{
	case PM_SETUID_REPLY:
	case PM_SETGID_REPLY:
	case PM_SETSID_REPLY:
	case PM_EXEC_REPLY:
	case PM_EXIT_REPLY:
	case PM_CORE_REPLY:
	case PM_FORK_REPLY:
	case PM_SRV_FORK_REPLY:
	case PM_UNPAUSE_REPLY:
	case PM_REBOOT_REPLY:
	case PM_SETGROUPS_REPLY:
		if (who_e == VFS_PROC_NR)
		{
			handle_vfs_reply();
			result= SUSPEND;		/* don't reply */
		}
		else
			result= ENOSYS;
		break;
	default:
		/* Else, if the system call number is valid, perform the
		 * call.
		 */
		if ((unsigned) call_nr >= NCALLS) {
			result = ENOSYS;
		} else {
#if ENABLE_SYSCALL_STATS
			calls_stats[call_nr]++;
#endif

			result = (*call_vec[call_nr])();

		}
		break;
	}

	/* Send reply. */
	if (result != SUSPEND) setreply(who_p, result);
	sendreply();
  }
  return(OK);
}

/*===========================================================================*
 *			       sef_local_startup			     *
 *===========================================================================*/
PRIVATE void sef_local_startup()
{
  /* Register init callbacks. */
  sef_setcb_init_fresh(sef_cb_init_fresh);
  sef_setcb_init_restart(sef_cb_init_fail);

  /* No live update support for now. */

  /* Register signal callbacks. */
  sef_setcb_signal_manager(sef_cb_signal_manager);

  /* Let SEF perform startup. */
  sef_startup();
}

/*===========================================================================*
 *		            sef_cb_init_fresh                                *
 *===========================================================================*/
PRIVATE int sef_cb_init_fresh(int type, sef_init_info_t *info)
{
/* Initialize the process manager. 
 * Memory use info is collected from the boot monitor, the kernel, and
 * all processes compiled into the system image. Initially this information
 * is put into an array mem_chunks. Elements of mem_chunks are struct memory,
 * and hold base, size pairs in units of clicks. This array is small, there
 * should be no more than 8 chunks. After the array of chunks has been built
 * the contents are used to initialize the hole list. Space for the hole list
 * is reserved as an array with twice as many elements as the maximum number
 * of processes allowed. It is managed as a linked list, and elements of the
 * array are struct hole, which, in addition to storage for a base and size in 
 * click units also contain space for a link, a pointer to another element.
*/
  int s;
  static struct boot_image image[NR_BOOT_PROCS];
  register struct boot_image *ip;
  static char core_sigs[] = { SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
				SIGEMT, SIGFPE, SIGBUS, SIGSEGV };
  static char ign_sigs[] = { SIGCHLD, SIGWINCH, SIGCONT };
  static char noign_sigs[] = { SIGILL, SIGTRAP, SIGEMT, SIGFPE, 
				SIGBUS, SIGSEGV };
  register struct mproc *rmp;
  register char *sig_ptr;
  message mess;

  /* Initialize process table, including timers. */
  for (rmp=&mproc[0]; rmp<&mproc[NR_PROCS]; rmp++) {
	init_timer(&rmp->mp_timer);
  }

  /* Build the set of signals which cause core dumps, and the set of signals
   * that are by default ignored.
   */
  (void) sigemptyset(&core_sset);
  for (sig_ptr = core_sigs; sig_ptr < core_sigs+sizeof(core_sigs); sig_ptr++)
	(void) sigaddset(&core_sset, *sig_ptr);
  (void) sigemptyset(&ign_sset);
  for (sig_ptr = ign_sigs; sig_ptr < ign_sigs+sizeof(ign_sigs); sig_ptr++)
	(void) sigaddset(&ign_sset, *sig_ptr);
  (void) sigemptyset(&noign_sset);
  for (sig_ptr = noign_sigs; sig_ptr < noign_sigs+sizeof(noign_sigs); sig_ptr++)
	(void) sigaddset(&noign_sset, *sig_ptr);

  /* Obtain a copy of the boot monitor parameters and the kernel info struct.  
   * Parse the list of free memory chunks. This list is what the boot monitor 
   * reported, but it must be corrected for the kernel and system processes.
   */
  if ((s=sys_getmonparams(monitor_params, sizeof(monitor_params))) != OK)
      panic("get monitor params failed: %d", s);
  if ((s=sys_getkinfo(&kinfo)) != OK)
      panic("get kernel info failed: %d", s);

  /* Initialize PM's process table. Request a copy of the system image table 
   * that is defined at the kernel level to see which slots to fill in.
   */
  if (OK != (s=sys_getimage(image))) 
  	panic("couldn't get image table: %d", s);
  procs_in_use = 0;				/* start populating table */
  for (ip = &image[0]; ip < &image[NR_BOOT_PROCS]; ip++) {
  	if (ip->proc_nr >= 0) {			/* task have negative nrs */
  		procs_in_use += 1;		/* found user process */

		/* Set process details found in the image table. */
		rmp = &mproc[ip->proc_nr];	
  		strncpy(rmp->mp_name, ip->proc_name, PROC_NAME_LEN); 
  		(void) sigemptyset(&rmp->mp_ignore);	
  		(void) sigemptyset(&rmp->mp_sigmask);
  		(void) sigemptyset(&rmp->mp_catch);
		if (ip->proc_nr == INIT_PROC_NR) {	/* user process */
  			/* INIT is root, we make it father of itself. This is
  			 * not really OK, INIT should have no father, i.e.
  			 * a father with pid NO_PID. But PM currently assumes 
  			 * that mp_parent always points to a valid slot number.
  			 */
  			rmp->mp_parent = INIT_PROC_NR;
  			rmp->mp_procgrp = rmp->mp_pid = INIT_PID;
			rmp->mp_flags |= IN_USE; 

			/* Set scheduling info */
			rmp->mp_scheduler = KERNEL;
			rmp->mp_nice = get_nice_value(USR_Q);
            rmp->mp_nicelim.rlim_cur = RLIM_NICE_DEFAULT;
		}
		else {					/* system process */
  			if(ip->proc_nr == RS_PROC_NR) {
  				rmp->mp_parent = INIT_PROC_NR;
  			}
  			else {
  				rmp->mp_parent = RS_PROC_NR;
  			}
  			rmp->mp_pid = get_free_pid();
			rmp->mp_flags |= IN_USE | PRIV_PROC;

			/* RS schedules this process */
			rmp->mp_scheduler = NONE;
			rmp->mp_nice = get_nice_value(SRV_Q);
            rmp->mp_nicelim.rlim_cur = RLIM_NICE_DEFAULT;
		}

		/* Get kernel endpoint identifier. */
		rmp->mp_endpoint = ip->endpoint;

		/* Tell VFS about this system process. */
		mess.m_type = PM_INIT;
		mess.PM_SLOT = ip->proc_nr;
		mess.PM_PID = rmp->mp_pid;
		mess.PM_PROC = rmp->mp_endpoint;
  		if (OK != (s=send(VFS_PROC_NR, &mess)))
			panic("can't sync up with VFS: %d", s);
  	}
  }

  /* Tell VFS that no more system processes follow and synchronize. */
  mess.PR_ENDPT = NONE;
  if (sendrec(VFS_PROC_NR, &mess) != OK || mess.m_type != OK)
	panic("can't sync up with VFS");

#if (CHIP == INTEL)
        uts_val.machine[0] = 'i';
        strcpy(uts_val.machine + 1, itoa(getprocessor()));
#endif  

 system_hz = sys_hz();

 /* Map out our own text and data. This is normally done in crtso.o
  * but PM is an exception - we don't get to talk to VM so early on.
  * That's why we override munmap() and munmap_text() in utility.c.
  *
  * _minix_unmapzero() is the same code in crtso.o that normally does
  * it on startup. It's best that it's there as crtso.o knows exactly
  * what the ranges are of the filler data.
  */
  unmap_ok = 1;
  _minix_unmapzero();

  /* Initialize user-space scheduling. */
  sched_init();

  return(OK);
}

/*===========================================================================*
 *		            sef_cb_signal_manager                            *
 *===========================================================================*/
PRIVATE int sef_cb_signal_manager(endpoint_t target, int signo)
{
/* Process signal on behalf of the kernel. */
  int r;

  r = process_ksig(target, signo);
  sendreply();

  return r;
}

/*===========================================================================*
 *				setreply				     *
 *===========================================================================*/
PUBLIC void setreply(proc_nr, result)
int proc_nr;			/* process to reply to */
int result;			/* result of call (usually OK or error #) */
{
/* Fill in a reply message to be sent later to a user process.  System calls
 * may occasionally fill in other fields, this is only for the main return
 * value, and for setting the "must send reply" flag.
 */
  register struct mproc *rmp = &mproc[proc_nr];

  if(proc_nr < 0 || proc_nr >= NR_PROCS)
      panic("setreply arg out of range: %d", proc_nr);

  rmp->mp_reply.reply_res = result;
  rmp->mp_flags |= REPLY;	/* reply pending */
}

/*===========================================================================*
 *				sendreply				     *
 *===========================================================================*/
PRIVATE void sendreply()
{
  int proc_nr;
  int s;
  struct mproc *rmp;

  /* Send out all pending reply messages, including the answer to
   * the call just made above.
   */
  for (proc_nr=0, rmp=mproc; proc_nr < NR_PROCS; proc_nr++, rmp++) {
      /* In the meantime, the process may have been killed by a
       * signal (e.g. if a lethal pending signal was unblocked)
       * without the PM realizing it. If the slot is no longer in
       * use or the process is exiting, don't try to reply.
       */
      if ((rmp->mp_flags & (REPLY | IN_USE | EXITING)) ==
          (REPLY | IN_USE)) {
          s=sendnb(rmp->mp_endpoint, &rmp->mp_reply);
          if (s != OK) {
              printf("PM can't reply to %d (%s): %d\n",
                  rmp->mp_endpoint, rmp->mp_name, s);
          }
          rmp->mp_flags &= ~REPLY;
      }
  }
}

/*===========================================================================*
 *				get_nice_value				     *
 *===========================================================================*/
PRIVATE int get_nice_value(queue)
int queue;				/* store mem chunks here */
{
/* Processes in the boot image have a priority assigned. The PM doesn't know
 * about priorities, but uses 'nice' values instead. The priority is between 
 * MIN_USER_Q and MAX_USER_Q. We have to scale between PRIO_MIN and PRIO_MAX.
 */ 
  int nice_val = (queue - USER_Q) * (PRIO_MAX-PRIO_MIN+1) / 
      (MIN_USER_Q-MAX_USER_Q+1);
  if (nice_val > PRIO_MAX) nice_val = PRIO_MAX;	/* shouldn't happen */
  if (nice_val < PRIO_MIN) nice_val = PRIO_MIN;	/* shouldn't happen */
  return nice_val;
}

void checkme(char *str, int line)
{
	struct mproc *trmp;
	int boned = 0;
	int proc_nr;
	for (proc_nr=0, trmp=mproc; proc_nr < NR_PROCS; proc_nr++, trmp++) {
		if ((trmp->mp_flags & (REPLY | IN_USE | EXITING)) ==
		   (REPLY | IN_USE)) {
			int tp;
  			if(pm_isokendpt(trmp->mp_endpoint, &tp) != OK) {
			   printf("PM: %s:%d: reply %d to %s is bogus endpoint %d after call %d by %d\n",
				str, line, trmp->mp_reply.m_type,
				trmp->mp_name, trmp->mp_endpoint, call_nr, who_e);
			   boned=1;
			}
		}
		if(boned) panic("corrupt mp_endpoint?");
	}
}

/*===========================================================================*
 *				handle_vfs_reply       			     *
 *===========================================================================*/
PRIVATE void handle_vfs_reply()
{
  struct mproc *rmp;
  endpoint_t proc_e;
  int r, proc_n;

  /* PM_REBOOT is the only request not associated with a process.
   * Handle its reply first.
   */
  if (call_nr == PM_REBOOT_REPLY) {
	vir_bytes code_addr;
	size_t code_size;

	/* Ask the kernel to abort. All system services, including
	 * the PM, will get a HARD_STOP notification. Await the
	 * notification in the main loop.
	 */
	code_addr = (vir_bytes) monitor_code;
	code_size = strlen(monitor_code) + 1;
	sys_abort(abort_flag, PM_PROC_NR, code_addr, code_size);

	return;
  }

  /* Get the process associated with this call */
  proc_e = m_in.PM_PROC;

  if (pm_isokendpt(proc_e, &proc_n) != OK) {
	panic("handle_vfs_reply: got bad endpoint from VFS: %d", proc_e);
  }

  rmp = &mproc[proc_n];

  /* Now that VFS replied, mark the process as VFS-idle again */
  if (!(rmp->mp_flags & VFS_CALL))
	panic("handle_vfs_reply: reply without request: %d", call_nr);

  rmp->mp_flags &= ~VFS_CALL;

  if (rmp->mp_flags & UNPAUSED)
  	panic("handle_vfs_reply: UNPAUSED set on entry: %d", call_nr);

  /* Call-specific handler code */
  switch (call_nr) {
  case PM_SETUID_REPLY:
  case PM_SETGID_REPLY:
  case PM_SETGROUPS_REPLY:
	/* Wake up the original caller */
	setreply(rmp-mproc, OK);

	break;

  case PM_SETSID_REPLY:
	/* Wake up the original caller */
	setreply(rmp-mproc, rmp->mp_procgrp);

	break;

  case PM_EXEC_REPLY:
	exec_restart(rmp, m_in.PM_STATUS, (vir_bytes)m_in.PM_PC);

	break;

  case PM_EXIT_REPLY:
	exit_restart(rmp, FALSE /*dump_core*/);

	break;

  case PM_CORE_REPLY:
	if (m_in.PM_STATUS == OK)
		rmp->mp_sigstatus |= DUMPED;

	exit_restart(rmp, TRUE /*dump_core*/);

	break;

  case PM_FORK_REPLY:
	/* Schedule the newly created process ... */
	r = (OK);
	if (rmp->mp_scheduler != KERNEL && rmp->mp_scheduler != NONE) {
		r = sched_start_user(rmp->mp_scheduler, rmp);
	}

	/* If scheduling the process failed, we want to tear down the process
	 * and fail the fork */
	if (r != (OK)) {
		/* Tear down the newly created process */
		rmp->mp_scheduler = NONE; /* don't try to stop scheduling */
		exit_proc(rmp, -1, FALSE /*dump_core*/);

		/* Wake up the parent with a failed fork */
		setreply(rmp->mp_parent, -1);

	}
	else {
		/* Wake up the child */
		setreply(proc_n, OK);

		/* Wake up the parent */
		setreply(rmp->mp_parent, rmp->mp_pid);
	}

	break;

  case PM_SRV_FORK_REPLY:
	/* Nothing to do */

	break;

  case PM_UNPAUSE_REPLY:
	/* Process is now unpaused */
	rmp->mp_flags |= UNPAUSED;

	break;

  default:
	panic("handle_vfs_reply: unknown reply code: %d", call_nr);
  }

  /* Now that the process is idle again, look at pending signals */
  if ((rmp->mp_flags & (IN_USE | EXITING)) == IN_USE)
	  restart_sigs(rmp);
}
