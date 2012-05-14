#include "fs.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/resource.h>
#include "fproc.h"
#include "param.h"

PUBLIC int do_getrlimit()
{
    register struct fproc *rfp;
    int s, resource;
    vir_bytes src, dst;
    struct rlimit rlim;
    
    rfp = fp;
    
    /* get resource type from message */
    resource = m_in.rlimit_resource; 

    /* Copy rlim structure to VFS */
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
        case RLIMIT_FSIZE:
            rlim = rfp->fp_fsizelim;
            break;
        case RLIMIT_NOFILE:
            rlim = rfp->fp_nofilelim;
            break;
    }
    
    /* Copy filled struc to user space */
    src = (vir_bytes) &rlim;
    dst = (vir_bytes) m_in.rlimit_struct;
    if((s=sys_datacopy(SELF, src, who_e, dst,
                    (phys_bytes) sizeof(struct rlimit))) != OK) return(s);

    return(OK);

}
