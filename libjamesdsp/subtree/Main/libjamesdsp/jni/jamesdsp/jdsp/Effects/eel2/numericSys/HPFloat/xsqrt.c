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
#include <stdio.h>
#include <math.h>

xpr
xsqrt (xpr z)
{
  xpr s, h;
  short m, e;
  unsigned short *pc;

  if ((xsigerr ((x_neg (&z)), XEDOM, "xsqrt()")))
    return xZero;
  else
    {
      pc = (unsigned short *) &z;
      if (*pc == 0)
	return xZero;
      e = *pc - xBias;
      *pc = xBias + (e % 2);
      e /= 2;
      s = dbltox (sqrt (xtodbl (z)));
      for (m = 0; m < xItt_div; ++m)
	{
	  h = xdiv (xadd (z, xmul (s, s), 1), xpr2 (s, 1));
	  s = xadd (s, h, 0);
	}
      pc = (unsigned short *) &s;
      *pc += e;
      return s;
    }
}
