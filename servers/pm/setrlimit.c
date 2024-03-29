#include <minix/com.h>
#include <minix/callnr.h>
#include <sys/resource.h>
#include "pm.h"
#include "mproc.h"
#include "param.h"
#include "unode.h"


PUBLIC int do_setrlimit()
{
    register struct mproc *rmp;
    register struct unode *run;
    int s, resource;
    vir_bytes src, dst;
    struct rlimit rlim;

    rmp = mp;
    run = un;
    
    /* Get the resource type from message */
    resource = m_in.rlimit_resource;

    /* Copy rlim structure to PM */
    if((struct rlimit*) m_in.rlimit_struct == (struct rlimit*) NULL)
    {
        return(EFAULT);
    }
    else
    {   
        src = (vir_bytes) m_in.rlimit_struct;
        dst = (vir_bytes) &rlim;
        if((s=sys_datacopy(who_e, src, SELF, dst,
                        (phys_bytes) sizeof(struct rlimit))) != OK) return(s);
    }   

    /* Check if the specified limit is valid  */
    if(rlim.rlim_cur != RLIM_INFINITY && rlim.rlim_cur < 0) 
    {
        return(EINVAL);
    }
    else if(rlim.rlim_max != RLIM_INFINITY) {
        /* rlim.rlim_cur must be different of 
           RLIM_INFINITY because the hard limit is smaller than RLIM_INFINITY 
        */
        if(rlim.rlim_max < rlim.rlim_cur || rlim.rlim_max < 0 || rlim.rlim_cur == RLIM_INFINITY) 
        {
            return(EINVAL);
        }
    }   

    switch(resource)
    {
          case RLIMIT_CPU:
            break;

        case RLIMIT_NICE:
            /* Check that value is in range and that max >= cur */
            if(!((rlim.rlim_cur <= (PRIO_MAX - PRIO_MIN) && rlim.rlim_max <= (PRIO_MAX - PRIO_MIN))
                    || (rlim.rlim_cur == RLIM_INFINITY && rlim.rlim_max == RLIM_INFINITY))) 
                return(EINVAL);
            /* Check that only super user can set a higher limit */
            if(rmp->mp_effuid != SUPER_USER && rmp->mp_nicelim.rlim_max != RLIM_INFINITY) { 
                    if(rlim.rlim_max > rmp->mp_nicelim.rlim_max || rlim.rlim_max == RLIM_INFINITY)
                        return(EPERM);
            }
            rmp->mp_nicelim = rlim;
            break;

        case RLIMIT_NPROC:
            run->plim = rlim;
            break;
    }

    return(OK);
}
