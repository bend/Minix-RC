#ifndef _SETRLIMIT_H_
#define _SETRLIMIT_H_

#include <stdlib.h>
#include <sys/resources.h>

_PROTOTYPE( int setrlimit, (int resource, const struct rlimit *rlim) );

#endif
