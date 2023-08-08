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

xpr
xmul (xpr s, xpr t)
{
  unsigned short pe[XDIM + 2], *q0, *q1, h;
  register unsigned short *pa, *pb, *pc;
  unsigned int m, n, p;
  short e;
  register short k;

  q0 = (unsigned short *) &s;
  q1 = (unsigned short *) &t;
  e = (*q0 & xM_exp) - xBias;
  k = (*q1 & xM_exp) + 1;
  if ((xsigerr (e > (short) xM_exp - k, XFPOFLOW, NULL)))
    return (((s.nmm[0] & xM_sgn) ^ (t.nmm[0] & xM_sgn)) ? xMinf : xPinf);
  if ((e += k) <= 0)
    return xZero;
  h = (*q0 ^ *q1) & xM_sgn;
  for (++q1, k = XDIM, p = n = 0L, pc = pe + XDIM + 1; k > 0; --k)
    {
      for (pa = q0 + k, pb = q1; pa > q0;)
	{
	  m = *pa--;
	  m *= *pb++;
	  n += (m & 0xffffL);
	  p += (m >> 16);
	}
      *pc-- = n;
      n = p + (n >> 16);
      p = 0L;
    }
  *pc = n;
  if (!(*pc & xM_sgn))
    {
      --e;
      if (e <= 0)
	return xZero;
      xlshift (1, pc, XDIM + 1);
    }
  if ((xsigerr (e == (short) xM_exp, XFPOFLOW, NULL)))
    return (!h ? xPinf : xMinf);
  *pe = e;
  *pe |= h;
  return *(xpr *) pe;
}
