#include "pm.h"
#include "mproc.h"
#include <stdio.h>

PUBLIC int do_getrlimit()
{
    printf("getrlimit sys call");
    return OK;
}
