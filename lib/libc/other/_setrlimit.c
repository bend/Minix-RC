#include <lib.h>
#include <unistd.h>
#include <setrlimit.h>

PUBLIC int setrlimit(void)
{
	message m;
	_syscall(PM_PROC_NR, SET_RLIMIT, &m);
}
