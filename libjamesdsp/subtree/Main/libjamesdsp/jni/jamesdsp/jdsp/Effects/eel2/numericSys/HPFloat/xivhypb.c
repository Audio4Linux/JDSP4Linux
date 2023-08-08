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
xatanh (xpr x)
{
  xpr y = x;

  y.nmm[0] &= xM_exp;		/* Now y == abs(x) */
  if ((xsigerr (xprcmp (&y, &xOne) >= 0, XEDOM, "xatanh")))
    return ((x.nmm[0] & xM_sgn) ? xMinf : xPinf);
  else
    {
      y = xdiv (xadd (xOne, x, 0), xadd (xOne, x, 1));
      return xpr2 (xlog (y), -1);
    }
}

xpr
xasinh (xpr x)
{
  xpr y = xmul (x, x);

  y = xsqrt (xadd (xOne, y, 0));
  if ((x.nmm[0] & xM_sgn))
    return xneg (xlog (xadd (y, x, 1)));
  else
    return xlog (xadd (x, y, 0));
}

xpr
xacosh (xpr x)
{
  if ((xsigerr (xprcmp (&x, &xOne) < 0, XEDOM, "xacosh()")))
    return xZero;
  else
    {
      xpr y = xmul (x, x);

      y = xsqrt (xadd (y, xOne, 1));
      return xlog (xadd (x, y, 0));
    }
}
