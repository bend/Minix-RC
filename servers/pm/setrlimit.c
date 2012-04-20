#include "pm.h"
#include "mproc.h"
#include <stdio.h>

PUBLIC int do_setrlimit()
{
    printf("setrlimit sys call");
    return OK;
}
