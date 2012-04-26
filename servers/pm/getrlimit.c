#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/resource.h>
#include "mproc.h"
#include "param.h"

PUBLIC int do_getrlimit()
{
    register struct mproc *rmp;
    int s, resource;
    vir_bytes src, dst;
    struct rlimit rlim;
    struct pnode *tmpnode;

    rmp = mp;
    
    /* Test if rlim is not null */
    if((struct rlimit*) m_in.rlimit_struct == (struct rlimit*) NULL) return(EFAULT);

    
    rlim.rlim_cur=100;
    rlim.rlim_max=200;



    src = (vir_bytes) &rlim;
    dst = (vir_bytes) m_in.rlimit_struct;
    if((s=sys_datacopy(SELF, src, who_e, dst,
                    (phys_bytes) sizeof(struct rlimit))) != OK) return(s);

    return(OK);

}
