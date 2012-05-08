#include <minix/com.h>
#include <minix/callnr.h>
#include <sys/resource.h>
#include "pm.h"
#include "mproc.h"
#include "param.h"


PUBLIC int do_setrlimit()
{
    register struct fproc *rfp;
    int s, resource;
    vir_bytes src, dst;
    struct rlimit rlim;
    
    rfp = fp;
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
                        (phys_bytes) sizeof(struct rlimit))) != OK) 
        {
            printf("sys copy failedi\n");
            return(s);
        }
    }   

    /* Check if the specified limit is valid  */
    if(rlim.rlim_cur != RLIM_INFINITY && rlim.rlim_cur < 0) 
    {
        printf("limit not valid\n");
        return(EINVAL);
    }
    else if(rlim.rlim_max != RLIM_INFINITY) {
        printf("!= rlimit_inf\n");
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
        case RLIMIT_FSIZE:
            if(rfp->fp_effuid != SUPER_USER && rfp->fp_fsizelim.rlim_max != RLIM_INFINITY) {
                if(rlim.rlim_max > rfp->fp_fsizelim.rlim_max || rlim.rlim_max == RLIM_INFINITY)
                    return(EPERM);
            }   

            rfp->fp_fsizelim = rlim;
            break;
        case RLIMIT_NOFILE:
            if(rfp->fp_effuid != SUPER_USER && rfp->fp_nofilelim.rlim_max != RLIM_INFINITY) {
                if(rlim.rlim_max > rfp->fp_nofilelim.rlim_max || rlim.rlim_max == RLIM_INFINITY) 
                    return(EPERM);
            } 
            rfp->fp_nofilelim = rlim;
            break;
    }

    return(OK);
}
