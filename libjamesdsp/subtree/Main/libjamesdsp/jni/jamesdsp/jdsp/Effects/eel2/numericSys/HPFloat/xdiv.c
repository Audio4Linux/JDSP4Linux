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
xdiv (xpr s, xpr t)
{
  xpr a;
  unsigned short *pc, e, i;

  pc = (unsigned short *) &t;
  e = *pc;
  *pc = xBias;
  if ((xsigerr (xprcmp (&t, &xZero) == 0, XEDIV, "xdiv()")))
    return xZero;
  else
    {
      a = dbltox (1 / xtodbl (t));
      *pc = e;
      pc = (unsigned short *) &a;
      *pc += xBias - (e & xM_exp);
      *pc |= e & xM_sgn;
      for (i = 0; i < xItt_div; ++i)
	a = xmul (a, xadd (xTwo, xmul (a, t), 1));
      return xmul (s, a);
    }
}
