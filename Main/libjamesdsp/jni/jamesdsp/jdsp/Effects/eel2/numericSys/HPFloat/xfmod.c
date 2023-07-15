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
  Heavily modified by Ivano Primi - 01/30/2005
*/

xpr
xfmod (xpr s, xpr t, xpr *q)
{
  if ((xsigerr (xprcmp (&t, &xZero) == 0, XEDIV, "xfmod()")))
    return xZero;
  else
    {
      unsigned short *p, mask = 0xffff;
      short e, i;
      int u;

      *q = xdiv (s, t);
      p = (unsigned short *) q;
      u = (*p & xM_sgn) ? 0 : 1;
      e = (*p &= xM_exp);	/* biased exponent of *q */
      e = e < xBias ? 0 : e - xBias + 1;
      for (i = 1; e / 16 > 0; i++, e -= 16);
      if (i <= XDIM)
	{
	  /* e = 0, ..., 15 */
	  mask <<= 16 - e;
	  p[i] &= mask;
	  for (i++; i <= XDIM; p[i] = 0, i++);
	}
      /* Now *q == abs(quotient of (s/t)) */
      return xadd (s, xmul (t, *q), u);
    }
}

xpr
xfrexp (xpr s, int *p)
{
  unsigned short *ps = (unsigned short *) &s, u;

  *p = (*ps & xM_exp) - xBias + 1;
  u = *ps & xM_sgn;
  *ps = xBias - 1;
  *ps |= u;
  return s;
}
