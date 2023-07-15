/*
   Copyright (C)  2000    Daniel A. Atkinson  <DanAtk@aol.com>
   Copyright (C)  2004    Ivano Primi  <ivprimi@libero.it>    

   This file is part of the HPA Library.

   The HPA Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HPA Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the HPA Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA.
*/

#ifndef _CXPRE_H_
#define _CXPRE_H_

#include<stdio.h>
#include"xpre.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    xpr re, im;
  } cxpr;

  typedef struct
  {
    int re, im;
  } cxprcmp_res;

  extern const cxpr cxZero;
  extern const cxpr cxOne;
  extern const cxpr cxIU;

  cxpr cxreset (xpr re, xpr im);
  cxpr cxconv (xpr x);
  xpr cxre (cxpr z);
  xpr cxim (cxpr z);
  cxpr cxswap (cxpr z);

  xpr cxabs (cxpr z);
  xpr cxarg (cxpr z);
  int cxrec (cxpr z, cxpr *w);

  cxpr cxadd (cxpr z1, cxpr z2, int k);
  cxpr cxsum (cxpr z1, cxpr z2);
  cxpr cxsub (cxpr z1, cxpr z2);
  cxpr cxmul (cxpr z1, cxpr z2);
  /* Multiplication by a real number */
  cxpr cxrmul (xpr c, cxpr z);

  /* Multiplication by +i */
  cxpr cxdrot (cxpr z);

  /* Multiplication by -i */
  cxpr cxrrot (cxpr z);
  cxpr cxdiv (cxpr z1, cxpr z2);

  cxpr cxgdiv (cxpr z1, cxpr z2);
  cxpr cxidiv (cxpr z1, cxpr z2);
  cxpr cxgmod (cxpr z1, cxpr z2);
  cxpr cxmod (cxpr z1, cxpr z2);
  cxpr cxpwr (cxpr z, int n);
  cxpr cxsqr (cxpr z);
  cxpr cxpow (cxpr z1, cxpr z2);
  cxpr cxroot (cxpr z, int i, int n);
  cxpr cxsqrt (cxpr z);

  cxprcmp_res cxprcmp (const cxpr* z1, const cxpr* z2);
  int cxis0 (const cxpr* z);
  int cxnot0 (const cxpr* z);
  int cxeq (cxpr z1, cxpr z2);
  int cxneq (cxpr z1, cxpr z2);
  int cxgt (cxpr z1, cxpr z2);
  int cxge (cxpr z1, cxpr z2);
  int cxlt (cxpr z1, cxpr z2);
  int cxle (cxpr z1, cxpr z2);

  cxpr cxconj (cxpr z);
  cxpr cxneg (cxpr z);
  cxpr cxinv (cxpr z);

  cxpr cxexp (cxpr z);
  cxpr cxexp10 (cxpr z);
  cxpr cxexp2 (cxpr z);
  cxpr cxlog (cxpr z);
  cxpr cxlog10 (cxpr z);
  cxpr cxlog2 (cxpr z);
  cxpr cxlog_sqrt (cxpr z);
  cxpr cxsin (cxpr z);
  cxpr cxcos (cxpr z);
  cxpr cxtan (cxpr z);
  cxpr cxsinh (cxpr z);
  cxpr cxcosh (cxpr z);
  cxpr cxtanh (cxpr z);
  cxpr cxasin (cxpr z);
  cxpr cxacos (cxpr z);
  cxpr cxatan (cxpr z);
  cxpr cxasinh (cxpr z);
  cxpr cxacosh (cxpr z);
  cxpr cxatanh (cxpr z);

  cxpr cxfloor (cxpr z);
  cxpr cxceil (cxpr z);
  cxpr cxround (cxpr z);
  cxpr cxtrunc (cxpr z);
  cxpr cxfrac (cxpr z);
  cxpr cxfix (cxpr z);

/* Conversion's functions */
  cxpr strtocx (const char *q, char **endptr);
  cxpr atocx (const char *s);
  char *cxpr_asprint (cxpr z, int sc_not, int sign, int lim);
  char *cxtoa (cxpr z, int lim);
  cxpr dctocx (double re, double im);
  cxpr fctocx (float re, float im);
  cxpr ictocx (long re, long im);
  cxpr uctocx (unsigned long re, unsigned long im);
  void cxtodc (const cxpr *z, double *re, double *im);
  void cxtofc (const cxpr *z, float *re, float *im);

/* Output functions */

#define CX1I_CHAR 'i'
#define CX1I_STR  "i"

  void cxpr_print (FILE * stream, cxpr z, int sc_not, int sign,
		   int lim);
  void cxprcxpr (cxpr z, int m);
  void cxprint (FILE * stream, cxpr z);

/* Special output functions and related macros */

#define XFMT_STD       0
#define XFMT_RAW       1
#define XFMT_ALT       2

#define CXDEF_LDEL   '('
#define CXDEF_RDEL   ')'
#define CX_SEPARATOR ", "	/* TO BE USED WITH THE ALT FORMAT */
#define CX_EMPTY_SEP "  "	/* TO BE USED WITH THE RAW FORMAT */
#define CX_SEP_L     2		/* LENGTH OF THE SEPARATOR        */

  int cxfout (FILE * pf, xoutflags ofs, cxpr z);
  int cxout (xoutflags ofs, cxpr z);
  unsigned long
    cxsout (char *s, unsigned long n, xoutflags ofs, cxpr z);

#define CXRESET(re, im) (cxpr){re, im}
#define CXCONV(x) (cxpr){x, xZero}
#define CXRE(z) (z).re
#define CXIM(z) (z).im
#define CXSWAP(z) (cxpr){(z).im, (z).re}

#define cxconvert cxconv
#define cxdiff cxsub
#define cxprod cxmul
#define cxipow cxpwr

#ifdef __cplusplus
}
#endif
#endif				/* _CXPRE_H_ */
