#include <lib.h>
#include <sys/resource.h>

PUBLIC int setrlimit(int resource, const struct rlimit *rlim)
{
    message m;

    m.m2_i1 = resource;
    m.m2_p1 = rlim;

    switch(resource)
    {
        case RLIMIT_CORE: 
        case RLIMIT_CPU:
        case RLIMIT_NICE:
        case RLIMIT_NPROC:
            return(_syscall(PM_PROC_NR, GET_RLIMIT, &m));
    
        case RLIMIT_FSIZE:
        case RLIMIT_NOFILE:
            return(_syscall(VFS_PROC_NR, GET_RLIMIT, &m));
        
        default:
            errno = EINVAL;
            return(-1);
    }
}

