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
xtanh (xpr z)
{
  xpr s, d, f;
  int m, k;

  if ((k = x_exp (&z)) > xK_tanh)
    {
      if (x_neg (&z))
	return xneg (xOne);
      else
	return xOne;
    }
  if (k < xK_lin)
    return z;
  ++k;
  if (k > 0)
    z = xpr2 (z, -k);
  s = xmul (z, z);
  f = xZero;
  for (d = inttox (m = xMS_hyp); m > 1;)
    {
      f = xdiv (s, xadd (d, f, 0));
      d = inttox (m -= 2);
    }
  f = xdiv (z, xadd (d, f, 0));
  for (; k > 0; --k)
    f = xdiv (xpr2 (f, 1), xadd (d, xmul (f, f), 0));
  return f;
}

xpr
xsinh (xpr z)
{
  int k;

  if ((k = x_exp (&z)) < xK_lin)
    return z;
  else if (k < 0)
    {
      z = xtanh (xpr2 (z, -1));
      return xdiv (xpr2 (z, 1), xadd (xOne, xmul (z, z), 1));
    }
  else
    {
      z = xexp (z);
      return xpr2 (xadd (z, xdiv (xOne, z), 1), -1);
    }
}

xpr
xcosh (xpr z)
{
  if (x_exp (&z) < xK_lin)
    return xOne;
  z = xexp (z);
  return xpr2 (xadd (z, xdiv (xOne, z), 0), -1);
}
