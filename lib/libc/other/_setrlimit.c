#include <lib.h>
#include <unistd.h>
#include <setrlimit.h>

PUBLIC void setrlimit(int resource, const struct rlimit *rlim)
{
	message m;
	_syscall(PM_PROC_NR, SET_RLIMIT, &m);
}
