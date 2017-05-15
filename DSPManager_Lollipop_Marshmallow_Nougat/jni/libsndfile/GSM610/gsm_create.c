#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gsm.h"
#include "gsm610_priv.h"

gsm gsm_create (void)
{
	gsm  r;

	r = malloc (sizeof(struct gsm_state));
	if (!r) return r;
	
	memset((char *)r, 0, sizeof (struct gsm_state));
	r->nrp = 40;

	return r;
}

/* Added for libsndfile : May 6, 2002. Not sure if it works. */
void gsm_init (gsm state)
{
	memset (state, 0, sizeof (struct gsm_state)) ;
	state->nrp = 40 ;
} 

