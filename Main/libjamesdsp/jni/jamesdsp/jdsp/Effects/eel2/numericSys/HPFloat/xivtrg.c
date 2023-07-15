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
#include <math.h>

xpr
xatan (xpr z)
{
  xpr s, f;
  int k, m;

  if ((k = x_exp (&z)) < xK_lin)
    return z;
  if (k >= 0)
    {
      /* k>=0 is equivalent to abs(z) >= 1.0 */
      z = xdiv (xOne, z);
      m = 1;
    }
  else
    m = 0;
  f = dbltox (atan (xtodbl (z)));
  s = xadd (xOne, xmul (z, z), 0);
  for (k = 0; k < xItt_div; ++k)
    f = xadd (f, xdiv (xadd (z, xtan (f), 1), s), 0);
  if (m)
    {
      if (x_neg (&f))
	return xadd (xneg (xPi2), f, 1);
      else
	return xadd (xPi2, f, 1);
    }
  else
    return f;
}

xpr
xasin (xpr z)
{
  xpr u = z;

  u.nmm[0] &= xM_exp;
  if ((xsigerr (xprcmp (&u, &xOne) > 0, XEDOM, "xasin()")))
    return ((x_neg (&z)) ? xneg (xPi2) : xPi2);
  else
    {
      if (x_exp (&z) < xK_lin)
	return z;
      u = xsqrt (xadd (xOne, xmul (z, z), 1));
      if (x_exp (&u) == -xBias)
	return ((x_neg (&z)) ? xneg (xPi2) : xPi2);
      return xatan (xdiv (z, u));
    }
}

xpr
xacos (xpr z)
{
  xpr u = z;

  u.nmm[0] &= xM_exp;
  if ((xsigerr (xprcmp (&u, &xOne) > 0, XEDOM, "xacos()")))
    return ((x_neg (&z)) ? xPi : xZero);
  else
    {
      if (x_exp (&z) == -xBias)
	return xPi2;
      u = xsqrt (xadd (xOne, xmul (z, z), 1));
      u = xatan (xdiv (u, z));
      if (x_neg (&z))
	return xadd (xPi, u, 0);
      else
	return u;
    }
}

/* Kindly added by A.Haumer 2010-04.09 */

xpr
xatan2 (xpr y, xpr x)
{
  int rs, is;

  rs = xsgn (&x);
  is = xsgn (&y);
  if (rs > 0)
    return xatan (xdiv (y, x));
  else if (rs < 0)
    {
      x.nmm[0] ^= xM_sgn;
      y.nmm[0] ^= xM_sgn;
      if (is >= 0)
	return xadd (xPi, xatan (xdiv (y, x)), 0);
      else
	return xadd (xatan (xdiv (y, x)), xPi, 1);
    }
  else				/* x is zero ! */
    {
      if (!xsigerr (is == 0, XEDOM, "xatan2()"))
	return (is > 0 ? xPi2 : xneg (xPi2));
      else
	return xZero;	        /* Dummy value :) */
    }
}
