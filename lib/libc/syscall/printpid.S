#include <machine/asm.h>

IMPORT(_printpid)
ENTRY(printpid)
	jmp	_C_LABEL(_printpid)

