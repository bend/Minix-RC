#include "pm.h"
#include "mproc.h"
#include <stdio.h>

PUBLIC int do_printpid()
{
    printf("%d\n", mp->mp_pid);
    return OK;
}
