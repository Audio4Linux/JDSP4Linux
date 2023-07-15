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
xexp2 (xpr x)
{
  xpr s, d, f;
  unsigned short *pf = (unsigned short *) &x;
  int m, k;

  if (xprcmp (&x, &xE2min) < 0)
    return xZero;
  else if ((xsigerr (xprcmp (&x, &xE2max) > 0, XFPOFLOW, NULL)))
    return xPinf;
  else
    {
      m = (*pf & xM_sgn) ? 1 : 0;
      x = xsfmod (x, &k);
      if ((m))
	k *= -1;
      /* -xBias <= k <= +xBias */
      x = xmul (x, xLn2);
      if (x_exp (&x) > -xBias)
	{
	  x = xpr2 (x, -1);
	  s = xmul (x, x);
	  f = xZero;
	  for (d = inttox (m = xMS_exp); m > 1; m -= 2, d = inttox (m))
	    f = xdiv (s, xadd (d, f, 0));
	  f = xdiv (x, xadd (d, f, 0));
	  f = xdiv (xadd (d, f, 0), xadd (d, f, 1));
	}
      else
	f = xOne;
      pf = (unsigned short *) &f;
      if (-k > *pf)
	return xZero;
      else
	{
	  *pf += k;
	  if ((xsigerr (*pf >= xM_exp, XFPOFLOW, NULL)))
	    return xPinf;
	  else
	    return f;
	}
    }
}

xpr
xexp (xpr z)
{
  return xexp2 (xmul (z, xLog2_e));
}

xpr
xexp10 (xpr z)
{
  return xexp2 (xmul (z, xLog2_10));
}
