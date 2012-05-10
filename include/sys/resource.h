#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

/* Priority range for the get/setpriority() interface.
 * It isn't a mapping on the internal minix scheduling
 * priority.
 */
#define PRIO_MIN	-20
#define PRIO_MAX	 20

#define PRIO_PROCESS	0
#define PRIO_PGRP	1
#define PRIO_USER	2

int getpriority(int, int);
int setpriority(int, int, int);

#include <sys/time.h>
#include <limits.h>

typedef unsigned long rlim_t;

#define RLIM_INFINITY ((rlim_t) -1)
#define RLIM_SAVED_CUR RLIM_INFINITY
#define RLIM_SAVED_MAX RLIM_INFINITY

/* Nice value is from 0 to 40 and will be set as NICE - 20. 20 is an average 
 */
#define RLIM_NICE_DEFAULT 20
#define RLIM_NPROC_DEFAULT  CHILD_MAX
#define RLIM_FSIZE_DEFAULT RLIM_INFINITY
#define RLIM_NOFILE_DEFAULT OPEN_MAX

struct rlimit
{
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

#define RLIMIT_CORE	1
#define RLIMIT_CPU	2
#define RLIMIT_DATA	3
#define RLIMIT_FSIZE	4
#define RLIMIT_NOFILE	5
#define RLIMIT_STACK	6
#define RLIMIT_AS	7
#define RLIMIT_NICE     8
#define RLIMIT_NPROC    9

#define RLIM_NLIMITS 8

int getrlimit(int resource, struct rlimit *rlp);

int setrlimit(int resource, const struct rlimit *rlp);

#endif
