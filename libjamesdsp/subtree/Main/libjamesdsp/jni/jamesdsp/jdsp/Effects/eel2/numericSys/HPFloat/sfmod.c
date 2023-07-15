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
xsfmod (xpr s, int *p)
{
  unsigned short *pa, *pb;
  short e, k;

  pa = (unsigned short *) &s;
  pb = pa + 1;
  e = (*pa & xM_exp) - xBias;
  if ((xsigerr (e >= 15, XFPOFLOW, NULL)))
    {
      *p = -1;
      return s;
    }
  else if (e < 0)
    {
      *p = 0;
      return s;
    }
  *p = *pb >> (15 - e);
  xlshift (++e, pb, XDIM);
  *pa -= e;
  for (e = 0; *pb == 0 && e < xMax_p; ++pb, e += 16);
  if (e == xMax_p)
    return xZero;
  for (k = 0; !((*pb << k) & xM_sgn); ++k);
  if ((k += e))
    {
      xlshift (k, pa + 1, XDIM);
      *pa -= k;
    }
  return s;
}
