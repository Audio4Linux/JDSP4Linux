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

#include"cxpre.h"		/* Automatically includes "xpre.h" */

cxpr
cxreset (xpr re, xpr im)
{
  cxpr y;

  y.re = re;
  y.im = im;
  return y;
/* MS-VC requires a more explicit coding,
   signaled on 2010-04-09 by A.Haumer

  return (cxpr)
  {
  re, im};
*/
}

cxpr
cxconv (xpr x)
{
  cxpr y;

  y.re = x;
  y.im = xZero;
  return y;
/* MS-VC requires a more explicit coding,
   signaled on 2010-04-09 by A.Haumer

  return (cxpr)
  {
  x, xZero};
*/
}

xpr
cxre (cxpr z)
{
  return z.re;
}

xpr
cxim (cxpr z)
{
  return z.im;
}

cxpr
cxswap (cxpr z)
{
  cxpr y;

  y.re = z.im;
  y.im = z.re;
  return y;
/* MS-VC requires a more explicit coding,
   signaled on 2010-04-09 by A.Haumer

  return (cxpr)
  {
  z.im, z.re};
*/
}

cxpr
cxneg (cxpr z)
{
  z.re.nmm[0] ^= xM_sgn;
  z.im.nmm[0] ^= xM_sgn;
  return z;
}

cxpr
cxconj (cxpr z)
{
  return (z.im.nmm[0] ^= xM_sgn, z);
}

#define XBOUND  XDIM * 16 + 8

xpr
cxabs (cxpr z)
{
  xpr x;
  int ea, eb;

  if (xprcmp (&z.re, &xZero) == 0 && xprcmp (&z.im, &xZero) == 0)
    return xZero;
  else
    {
      ea = (z.re.nmm[0] &= xM_exp) - xBias;
      eb = (z.im.nmm[0] &= xM_exp) - xBias;
      if (ea > eb + XBOUND)
	return z.re;
      else if (eb > ea + XBOUND)
	return z.im;
      else
	{
	  z.re.nmm[0] -= eb;
	  z.im.nmm[0] = xBias;
	  x = xsqrt (xadd (xmul (z.re, z.re), xmul (z.im, z.im), 0));
	  x.nmm[0] += eb;
	  return x;
	}
    }
}

xpr
cxarg (cxpr z)
{
  int rs, is;

  rs = xsgn (&z.re);
  is = xsgn (&z.im);
  if (rs > 0)
    return xatan (xdiv (z.im, z.re));
  else if (rs < 0)
    {
      z.re.nmm[0] ^= xM_sgn;
      z.im.nmm[0] ^= xM_sgn;
      if (is >= 0)
	return xadd (xPi, xatan (xdiv (z.im, z.re)), 0);
      else
	return xadd (xatan (xdiv (z.im, z.re)), xPi, 1);
    }
  else				/* z.re is zero ! */
    {
      if (!xsigerr (is == 0, XEDOM, "cxarg()"))
	return (is > 0 ? xPi2 : xneg (xPi2));
      else
	return xneg (xPi2);	/* Dummy value :) */
    }
}

int
cxrec (cxpr z, cxpr *w)
{
  xpr x;
  int sa, sb, ea, eb;

  if (xprcmp (&z.re, &xZero) == 0 && xprcmp (&z.im, &xZero) == 0)
    return 0;
  else
    {
      sa = z.re.nmm[0] & xM_sgn;
      sb = z.im.nmm[0] & xM_sgn;
      ea = (z.re.nmm[0] &= xM_exp) - xBias;
      eb = (z.im.nmm[0] &= xM_exp) - xBias;
      if (ea > eb + XBOUND)
	x = z.re;
      else if (eb > ea + XBOUND)
	x = z.im;
      else
	{
	  z.re.nmm[0] -= eb;
	  z.im.nmm[0] = xBias;
	  x = xsqrt (xadd (xmul (z.re, z.re), xmul (z.im, z.im), 0));
	  x.nmm[0] += eb;
	  z.re.nmm[0] += eb;
	  z.im.nmm[0] += eb;
	}
      w->re = xdiv (xdiv (z.re, x), x);
      w->im = xdiv (xdiv (z.im, x), x);
      w->re.nmm[0] |= sa;
      w->im.nmm[0] |= xM_sgn ^ sb;
      return 1;
    }
}

cxpr
cxfloor (cxpr z)
{
  cxpr w;

  w.re = xfloor (z.re);
  w.im = xfloor (z.im);
  return w;
}

cxpr
cxceil (cxpr z)
{
  cxpr w;

  w.re = xceil (z.re);
  w.im = xceil (z.im);
  return w;
}

cxpr
cxround (cxpr z)
{
  cxpr w;

  w.re = xround (z.re);
  w.im = xround (z.im);
  return w;
}

cxpr
cxtrunc (cxpr z)
{
  cxpr w;

  w.re = xtrunc (z.re);
  w.im = xtrunc (z.im);
  return w;
}

cxpr
cxfrac (cxpr z)
{
  cxpr w;

  w.re = xfrac (z.re);
  w.im = xfrac (z.im);
  return w;
}

cxpr
cxfix (cxpr z)
{
  cxpr w;

  w.re = xfix (z.re);
  w.im = xfix (z.im);
  return w;
}
