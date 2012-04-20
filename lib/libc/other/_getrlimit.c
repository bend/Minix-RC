#include <lib.h>
#include <unistd.h>
#include <getrlimit.h>

PUBLIC int getrlimit(int resource, struct rlimit *rlim)
{
	message m;
	_syscall(PM_PROC_NR, SET_RLIMIT, &m);
}
