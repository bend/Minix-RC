#include <lib.h>
#include <unistd.h>
#include <printpid.h>

PUBLIC void printpid()
{
    message m;
    _syscall(PM_PROC_NR, PRINTPID, &m);
}
