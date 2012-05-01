#include <lib.h>
#include <unistd.h>
#include <sys/resource.h>

PUBLIC int getrlimit(int resource, struct rlimit *rlim)
{

    message m;

    m.m2_i1 = resource;
    m.m2_p1 = rlim;

    printf("in getrlimit\n");

    switch(resource)
    {
        case RLIMIT_CORE:
        case RLIMIT_CPU:
        case RLIMIT_NICE:
        case RLIMIT_NPROC:
            printf("PM\n");
            return(_syscall(PM_PROC_NR, GET_RLIMIT, &m));
    
        case RLIMIT_FSIZE:
        case RLIMIT_NOFILE:
            printf("VFS");
            return(_syscall(VFS_PROC_NR, GET_RLIMIT, &m));
        
        default:
            errno = EINVAL;
            return(-1);
    }



}
