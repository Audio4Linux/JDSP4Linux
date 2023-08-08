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
xpwr (xpr s, int n)
{
  xpr t;
  unsigned k, m;

  t = xOne;
  if (n < 0)
    {
      m = -n;
      if ((xsigerr (xprcmp (&s, &xZero) == 0, XEBADEXP, "xpwr()")))
	return xZero;
      s = xdiv (xOne, s);
    }
  else
    m = n;
  if ((m))
    {
      k = 1;
      while (1)
	{
	  if ((k & m))
	    t = xmul (s, t);
	  if ((k <<= 1) <= m)
	    s = xmul (s, s);
	  else
	    break;
	}
    }
  else
    xsigerr (xprcmp (&s, &xZero) == 0, XEBADEXP, "xpwr()");
  return t;
}

xpr
xpr2 (xpr s, int m)
{
  unsigned short *p = (unsigned short *) &s;
  long e;

  for (e = 1; e <= XDIM && p[e] == 0; e++);
  if (e <= XDIM)
    {
      e = *p & xM_exp;		/* biased exponent */
      if (e + m < 0)
	return xZero;
      else if ((xsigerr (e + m >= xM_exp, XFPOFLOW, NULL)))
	return ((s.nmm[0] & xM_sgn) ? xMinf : xPinf);
      else
	{
	  *p += m;
	  return s;
	}
    }
  else				/* s is zero or +-Inf */
    return s;
}

xpr
xpow (xpr x, xpr y)
{
  if (xsigerr ((xis0(&x)) || x_exp (&x) == -xBias, XEDOM, "xpow()"))
    return xZero;
  else if (x_neg(&x))
	  return xexp2(xmul(xlog2(xneg(x)), y));
  else
    return xexp2 (xmul (xlog2 (x), y));
}
