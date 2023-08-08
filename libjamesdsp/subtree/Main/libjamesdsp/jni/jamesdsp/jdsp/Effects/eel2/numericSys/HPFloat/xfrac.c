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
  All the functions in this file have been created by
  Ivano Primi - 12/11/2004
*/

/*
  After skipping the first 'skip' bites of the vector 'p',
  this function nullifies all the remaining ones. 'k' is
  the number of words forming the vector p.
  Warning: 'skip' must be positive !
*/

static void
nullify (int skip, unsigned short *p, int k)
{
  int i;
  unsigned short mask = 0xffff;

  for (i = 0; skip / 16 > 0; i++, skip -= 16);
  if (i < k)
    {
      /* skip = 0, ..., 15 */
      mask <<= 16 - skip;
      p[i] &= mask;
      for (i++; i < k; p[i] = 0, i++);
    }
}

static void
canonic_form (xpr *px)
{
  unsigned short *p, u;
  short e, i, j, skip;

  p = (unsigned short *) px;
  e = (*p & xM_exp);		/* biased exponent of x */
  u = (*p & xM_sgn);		/* sign of x            */
  if (e < xBias - 1)
    return;
  else
    {
      unsigned short mask = 0xffff;

      /* e >= xBias - 1 */
      for (i = 1, skip = e + 1 - xBias; skip / 16 > 0; i++, skip -= 16);
      if (i <= XDIM)
	{
	  /* skip = 0, ..., 15 */
	  mask >>= skip;
	  if ((p[i] & mask) != mask)
	    return;
	  else
	    {
	      for (j = i + 1; j <= XDIM && p[j] == 0xffff; j++);
	      if (j > XDIM)
		{
		  p[i] -= mask;
		  for (j = i + 1; j <= XDIM; p[j] = 0, j++);
		  if (!(p[1] & 0x8000))
		    {
		      p[1] = 0x8000;
		      *p = ++e;
		      *p |= u;
		    }
		  else if ((u))
		    *px = xadd (*px, xOne, 1);
		  else
		    *px = xadd (*px, xOne, 0);
		}
	    }
	}			/* end   if(i <= XDIM ) */
    }				/* end outer else */
}

/*
  xfrac(x) returns the fractional part of the number x.
  xfrac(x) has the same sign as x.
*/

xpr
xfrac (xpr x)
{
  unsigned short u, *p;
  short e;
  int n;

  canonic_form (&x);
  p = (unsigned short *) &x;
  e = (*p & xM_exp);		/* biased exponent of x */
  if (e < xBias)
    return x;			/* The integer part of x is zero */
  else
    {
      u = *p & xM_sgn;		/* sign of x */
      n = e - xBias + 1;
      xlshift (n, p + 1, XDIM);
      e = xBias - 1;
      /* Now I have to take in account the rule */
      /* of the leading one.                    */
      while (e > 0 && !(p[1] & xM_sgn))
	{
	  xlshift (1, p + 1, XDIM);
	  e -= 1;
	}
      /* Now p+1 points to the fractionary part of x, */
      /* u is its sign, e is its biased exponent.     */
      p[0] = e;
      p[0] |= u;
      return *(xpr *) p;
    }
}


/*
  xtrunc(x) returns the integer part of the number x.
  xtrunc(x) has the same sign as x.
*/

xpr
xtrunc (xpr x)
{
  unsigned short *p;
  short e;

  canonic_form (&x);
  p = (unsigned short *) &x;
  e = (*p & xM_exp);		/* biased exponent of x */
  if (e < xBias)
    return xZero;		/* The integer part of x is zero */
  else
    {
      nullify (e - xBias + 1, p + 1, XDIM);
      return *(xpr *) p;
    }
}

xpr
xround (xpr x)
{
  return xtrunc (xadd (x, xRndcorr, x.nmm[0] & xM_sgn));
}

xpr
xceil (xpr x)
{
  unsigned short *ps = (unsigned short *) &x;

  if ((*ps & xM_sgn))
    return xtrunc (x);
  else
    {
      xpr y = xfrac (x);
      /* y has the same sign as x (see above). */

      return (xprcmp (&y, &xZero) > 0 ? xadd (xtrunc (x), xOne, 0) : x);
    }
}

xpr
xfloor (xpr x)
{
  unsigned short *ps = (unsigned short *) &x;

  if ((*ps & xM_sgn))
    {
      xpr y = xfrac (x);
      /* y has the same sign as x (see above). */

      return (xprcmp (&y, &xZero) < 0 ? xadd (xtrunc (x), xOne, 1) : x);
    }
  else
    return xtrunc (x);
}

static void
xadd_correction (xpr* px, int k)
{
  short e = (px->nmm[0] & xM_exp) - xBias;

/*   e = (e > 0 ? e : 0);   */
  *px = xadd (*px, xpr2 (xFixcorr, e), k);
}

xpr 
xfix (xpr x)
{
  unsigned short *p;
  short e;

  xadd_correction (&x, x.nmm[0] & xM_sgn);
  p = (unsigned short *) &x;
  e = (*p & xM_exp);		/* biased exponent of x */
  if (e < xBias)
    return xZero;		/* The integer part of x is zero */
  else
    {
      nullify (e - xBias + 1, p + 1, XDIM);
      return *(xpr *) p;
    }  
}
