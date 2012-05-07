#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/resource.h>
#include "mproc.h"
#include "param.h"
#include "unode.h"

PUBLIC int do_getrlimit()
{
    register struct mproc *rmp;
    int s, resource;
    vir_bytes src, dst;
    struct rlimit rlim;

    rmp = mp;
    
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


    switch(resource)
    {   
        case RLIMIT_CPU:
            break;
        case RLIMIT_NICE:
            rlim = rmp->mp_nicelim;
            printf("%d\n", rlim.rlim_cur);
            printf("%d\n", rlim.rlim_max);
            break;
        case RLIMIT_NPROC:
            printf("Will get nproc \n");
            rlim = un->plim;
            break;
    }



    src = (vir_bytes) &rlim;
    dst = (vir_bytes) m_in.rlimit_struct;
    if((s=sys_datacopy(SELF, src, who_e, dst,
                    (phys_bytes) sizeof(struct rlimit))) != OK) return(s);

    return(OK);

}
