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

#include <stdio.h>
#include "xpre.h"

xpr
xlog (xpr z)
{
  xpr f, h;
  int k, m;

  if ((xsigerr ((x_neg (&z)) || x_exp (&z) == -xBias, XEDOM, "xlog()")))
    return xMinf;
  else if (xprcmp (&z, &xOne) == 0)
    return xZero;
  else
    {
      z = xfrexp (z, &m);
      z = xmul (z, xSqrt2);
      z = xdiv (xadd (z, xOne, 1), xadd (z, xOne, 0));
      h = xpr2 (z, 1);
      z = xmul (z, z);
      for (f = h, k = 1; x_exp (&h) > -xMax_p;)
	{
	  h = xmul (h, z);
	  f = xadd (f, xdiv (h, inttox (k += 2)), 0);
	}
      return xadd (f, xmul (xLn2, dbltox (m - .5)), 0);
    }
}

xpr
xlog2 (xpr z)
{
  xpr f, h;
  int k, m;

  if ((xsigerr ((x_neg (&z)) || x_exp (&z) == -xBias, XEDOM, "xlog2()")))
    return xMinf;
  else if (xprcmp (&z, &xOne) == 0)
    return xZero;
  else
    {
      z = xfrexp (z, &m);
      z = xmul (z, xSqrt2);
      z = xdiv (xadd (z, xOne, 1), xadd (z, xOne, 0));
      h = xpr2 (z, 1);
      z = xmul (z, z);
      for (f = h, k = 1; x_exp (&h) > -xMax_p;)
	{
	  h = xmul (h, z);
	  f = xadd (f, xdiv (h, inttox (k += 2)), 0);
	}
      return xadd (xmul (f, xLog2_e), dbltox (m - .5), 0);
    }
}

xpr
xlog10 (xpr z)
{
  xpr w = xlog(z);
  
  if (xprcmp (&w, &xMinf) <= 0)
    return xMinf;
  else
    return xmul (w, xLog10_e);
}
