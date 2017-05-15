#include "gsm.h"
#include "config.h"

#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#else
#	ifdef	HAS_MALLOC_H
#		include 	<malloc.h>
#	else
		extern void free();
#	endif
#endif

void gsm_destroy (gsm S)
{
	if (S) free((char *)S);
}

