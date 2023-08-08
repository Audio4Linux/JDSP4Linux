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

#include "xpre.h"

/*

  Modified by Ivano Primi - 11/21/2004 and 03/05/2005

*/

int
xprcmp (const xpr *pa, const xpr *pb)
{
  register unsigned short e, k, *p, *q, p0, q0;
  register int m;

  p = (unsigned short *) pa;
  q = (unsigned short *) pb;
  for (m = 1; m <= XDIM && p[m] == 0; m++);
  if (m > XDIM && (*p & xM_exp) < xM_exp)
    /* *pa is actually zero */
    p0 = 0;
  else
    p0 = *p;
  for (m = 1; m <= XDIM && q[m] == 0; m++);
  if (m > XDIM && (*q & xM_exp) < xM_exp)
    /* *pb is actually zero */
    q0 = 0;
  else
    q0 = *q;
  e = p0 & xM_sgn;
  k = q0 & xM_sgn;
  if (e && !k)
    return -1;
  else if (!e && k)
    return 1;
  else				/* *pa and *pb have the same sign */
    {
      m = (e) ? -1 : 1;
      e = p0 & xM_exp;
      k = q0 & xM_exp;
      if (e > k)
	return m;
      else if (e < k)
	return -m;
      else
	{
	  for (e = 0; *(++p) == *(++q) && e < XDIM; ++e);
	  if (e < XDIM)
	    return (*p > *q ? m : -m);
	  else
	    return 0;
	}
    }
}

int
xeq (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) == 0);
}

int
xneq (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) != 0);
}

int
xgt (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) > 0);
}
  
int
xge (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) >= 0);
}

int
xlt (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) < 0);
}

int
xle (xpr x1, xpr x2)
{
  return (xprcmp (&x1, &x2) <= 0);
}

/*
  xisNaN (&x) returns 1 if and only if x is not a valid number
*/

int
xisNaN (const xpr *u)
{
  register unsigned short* p = (unsigned short *) u;

  if ((*p))
    return 0;
  else
    {
      register int i;

      for (i=1; i <= XDIM && p[i] == 0x0; i++);
      return (i <= XDIM ? 1 : 0);
    }
}

int
xis0 (const xpr *u)
{
  register unsigned short* p = (unsigned short *) u;
  register int m;

  for (m = 1; m <= XDIM && p[m] == 0; m++);
  return (m > XDIM && (*p & xM_exp) < xM_exp ? 1 : 0);
}

int
xnot0 (const xpr *u)
{
  register unsigned short* p = (unsigned short *) u;
  register int m;

  for (m = 1; m <= XDIM && p[m] == 0; m++);
  return (m > XDIM && (*p & xM_exp) < xM_exp ? 0 : 1);
}

int
xsgn (const xpr *u)
{
  register unsigned short* p = (unsigned short *) u;
  register int m;

  for (m = 1; m <= XDIM && p[m] == 0; m++);
  if ((m > XDIM && (*p & xM_exp) < xM_exp) || !*p)
    return 0;
  else
    return ((*p & xM_sgn) ? -1 : 1);
}

int
xisPinf (const xpr *u)
{
  return (*u->nmm == xM_exp ? 1 : 0);
}

int
xisMinf (const xpr *u)
{
  return (*u->nmm == (xM_exp | xM_sgn) ? 1 : 0);
}

int
xisordnumb (const xpr *u)
{
  int isNaN, isfinite;
  register unsigned short* p = (unsigned short *) u;

  if ((*p))
    isNaN = 0;
  else
    {
      register int i;

      for (i=1; i <= XDIM && p[i] == 0x0; i++);
      isNaN = i <= XDIM;
    }
  isfinite = (*p & xM_exp) < xM_exp;
  return (!isNaN && (isfinite) ? 1 : 0);
}
