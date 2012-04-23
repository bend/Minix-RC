#ifndef _GETRLIMIT_H_
#define _GETRLIMIT_H_

#include <stdlib.h>
#include <sys/resource.h>

_PROTOTYPE( int getrlimit, (int resource, struct rlimit *lim) );

#endif
