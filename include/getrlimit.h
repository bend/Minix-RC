#ifndef _GETRLIMIT_H_
#define _GETRLIMIT_H_

#include <stdlib.h>
#include <sys/resources.h>

_PROTOTYPE( int getrlimit, (int resource, struct rlimit *rlim) );

#endif
