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

#ifndef _XPRE_H_
#define _XPRE_H_

#include "hpaconf.h"

/* This macro has been added by Ivano Primi - 12/21/2004 */
#define XMAX_10EX  4931

/* This macro has been added by Ivano Primi - 05/10/2004 */
/* It is used in the file xchcof.c                       */
#define XMAX_DEGREE 50

/* Dealing with errors. Added by Ivano Primi - 01/04/2005 */
#ifndef XERR_IGN

#define XENONE   0
#define XEDIV    1
#define XEDOM    2
#define XEBADEXP 3
#define XFPOFLOW 4		/* Floating point overflow */

#define XNERR    4
#define XEINV    5		/* == XNERR + 1 */

#endif /* !XERR_IGN */

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    unsigned short nmm[XDIM + 1];
  } xpr;

  extern const unsigned short xM_sgn, xM_exp;
  extern const short xBias;
  extern const int xItt_div, xK_tanh;
  extern const int xMS_exp, xMS_trg, xMS_hyp;
  extern const short xMax_p, xK_lin;
  extern const short xD_bias, xD_max, xD_lex;
  extern const short xF_bias, xF_max, xF_lex;
  extern const xpr xZero, xOne, xTwo, xTen;
  extern const xpr xPinf, xMinf, xNaN;
  extern const xpr xPi, xPi2, xPi4, xEe, xSqrt2;
  extern const xpr xLn2, xLn10, xLog2_e, xLog2_10, xLog10_e;
  extern const xpr xRndcorr, xFixcorr;
  extern const xpr xVSV, xVGV, xEmax, xEmin, xE2min, xE2max, HPA_MIN, HPA_MAX;

  xpr xadd (xpr a, xpr b, int k);
  xpr xmul (xpr s, xpr t);
  xpr xdiv (xpr s, xpr t);
/* strtox() has been added by Ivano Primi - 12/21/2004 */
  xpr strtox (const char *q, char **endptr);
  xpr atox (const char *s);
  xpr dbltox (double y);
/* flttox() has been added by Ivano Primi - 11/22/2004 */
  xpr flttox (float y);
  xpr inttox (long n);
  xpr uinttox (unsigned long n);
  int xprcmp (const xpr *p, const xpr *q);
  int xeq (xpr x1, xpr x2); 
  int xneq (xpr x1, xpr x2);
  int xgt (xpr x1, xpr x2);  
  int xge (xpr x1, xpr x2); 
  int xlt (xpr x1, xpr x2);
  int xle (xpr x1, xpr x2);  
  int xisNaN (const xpr *u);
  int xisPinf (const xpr *u);
  int xisMinf (const xpr *u);
  int xisordnumb (const xpr *u);
  int xis0 (const xpr *u);
  int xnot0 (const xpr *u);
  int xsgn (const xpr *u);
  int x_neg (const xpr *p);
  int x_exp (const xpr *p);
  xpr xsfmod (xpr t, int *p);
  xpr xpwr (xpr s, int n);
  xpr xpr2 (xpr s, int n);
  xpr xneg (xpr s);
  xpr xabs (xpr s);
  xpr xfrexp (xpr s, int *p);
/* xfmod() has been modified by Ivano Primi - 01/30/2005 */
  xpr xfmod (xpr s, xpr t, xpr *q);
/* xfrac() and xtrunc() have been added by Ivano Primi - 12/11/2004 */
  xpr xfrac (xpr x);
  xpr xtrunc (xpr x);
/* xround(), xceil() and xfloor() have been added by Ivano Primi - 01/05/2004 */
  xpr xround (xpr x);
  xpr xceil (xpr x);
  xpr xfloor (xpr x);
/* xfix() has been added by Ivano Primi - 05/01/2004 */
  xpr xfix (xpr x);

  double xtodbl (xpr s);
/* xtoflt() has been added by Ivano Primi - 11/22/2004 */
  float xtoflt (xpr s);
  xpr xtan (xpr x);
  xpr xsin (xpr x);
  xpr xcos (xpr x);
  xpr xatan (xpr a);
  xpr xasin (xpr a);
  xpr xacos (xpr a);
  xpr xatan2 (xpr y, xpr x);
  xpr xsqrt (xpr u);
  xpr xexp (xpr u);
  xpr xexp2 (xpr u);
  xpr xexp10 (xpr u);
  xpr xlog (xpr u);
  xpr xlog2 (xpr u);
  xpr xlog10 (xpr u);
  xpr xtanh (xpr v);
  xpr xsinh (xpr v);
  xpr xcosh (xpr v);
  xpr xatanh (xpr v);
  xpr xasinh (xpr v);
  xpr xacosh (xpr v);
  xpr xpow (xpr x, xpr y);

  xpr* xchcof (int m, xpr (*xfunc) (xpr));
  xpr xevtch (xpr z, xpr *a, int m);
/* The following 4 functions have been added */
/* by Ivano Primi 11/29/2004                 */

#include<stdio.h>

  void xpr_print (FILE * stream, xpr u, int sc_not, int sign, int lim);
  char *xpr_asprint (xpr u, int sc_not, int sign, int lim);
  char *xtoa (xpr u, int lim);
  void xbprint (FILE * stream, xpr u);
  void xprxpr (xpr u, int m);
  void xprint (FILE * stream, xpr u);
  void xlshift (int i, unsigned short *p, int k);
  void xrshift (int i, unsigned short *p, int k);

/* The next special output functions have been added by Ivano Primi, */
/* 01/06/2005.                                                       */

#define XOUT_FIXED      0
#define XOUT_SCIENTIFIC 1

#define XDEF_LIM        6

  typedef struct
  {
    short fmt, notat, sf, mfwd, lim;
    signed char padding, ldel, rdel;
  } xoutflags;

/* Special output functions */
/* Remark: xfout(), xout() and xsout() actually ignore the fields ofs.fmt,  */
/* ofs.ldel and ofs.rdel .                                                  */
/* These fields are only used by cxfout(),cxout() and cxsout()(see cxpre.h).*/
  int xfout (FILE * pf, xoutflags ofs, xpr x);
  int xout (xoutflags ofs, xpr x);
  unsigned long
    xsout (char *s, unsigned long n, xoutflags ofs, xpr x);

#ifndef XERR_IGN
  int xsigerr (int errcond, int errcode, const char *where);
#else
#define xsigerr(errcond, errcode, where) 0
#endif

#define xsum(a, b) xadd (a, b, 0)
#define xsub(a, b) xadd (a, b, 1)

#ifdef __cplusplus
}
#endif
#endif				/* _XPRE_H_ */
