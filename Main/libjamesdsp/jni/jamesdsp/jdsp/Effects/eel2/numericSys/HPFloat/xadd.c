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
xadd (xpr s, xpr t, int f)
{
  unsigned short pe[XDIM + 1], h, u;
  register unsigned short *pa, *pb, *pc, *pf = pe;
  register unsigned int n = 0;
  short e;
  register short k;

  pa = (unsigned short *) &s;
  pb = (unsigned short *) &t;
  e = *pa & xM_exp;
  k = *pb & xM_exp;
  if (f)
    *pb ^= xM_sgn;
  u = (*pb ^ *pa) & xM_sgn;
  f = 0;
  if (e > k)
    {
      if ((k = e - k) >= xMax_p)
	return s;
      xrshift (k, pb + 1, XDIM);
    }
  else if (e < k)
    {
      if ((e = k - e) >= xMax_p)
	return t;
      xrshift (e, pa + 1, XDIM);
      e = k;
      pc = pa;
      pa = pb;
      pb = pc;
    }
  else if (u)
    {
      for (pc = pa, pf = pb; *(++pc) == *(++pf) && f < XDIM; ++f);
      if (f >= XDIM)
	return xZero;
      if (*pc < *pf)
	{
	  pc = pa;
	  pa = pb;
	  pb = pc;
	}
      pf = pe + f;
    }
  h = *pa & xM_sgn;
  if (u)
    {
      for (pc = pb + XDIM; pc > pb; --pc)
	*pc = ~(*pc);
      n = 1L;
    }
  for (pc = pe + XDIM, pa += XDIM, pb += XDIM; pc > pf;)
    {
      n += *pa;
      pa--;
      n += *pb;
      pb--;
      *pc = n;
      pc--;
      n >>= 16;
    }
  if (u)
    {
      for (; *(++pc) == 0; ++f);
      for (k = 0; !((*pc << k) & xM_sgn); ++k);
      if ((k += 16 * f))
	{
	  if ((e -= k) <= 0)
	    return xZero;
	  xlshift (k, pe + 1, XDIM);
	}
    }
  else
    {
      if (n)
	{
	  ++e;
	  if ((xsigerr (e == (short) xM_exp, XFPOFLOW, NULL)))
	    return (!h ? xPinf : xMinf);
	  ++pf;
	  xrshift (1, pf, XDIM);
	  *pf |= xM_sgn;
	}
    }
  *pe = e;
  *pe |= h;
  return *(xpr *) pe;
}
