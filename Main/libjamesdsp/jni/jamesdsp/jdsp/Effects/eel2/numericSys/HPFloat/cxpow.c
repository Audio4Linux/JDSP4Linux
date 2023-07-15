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

static void
unit_root (int i, int n, xpr *a, xpr *b)
{
  /*
     We assume n != 0
   */
  i %= n;
  *a = xdiv (xmul (xpr2 (inttox (i), 1), xPi), inttox (n));
  *b = xsin (*a);
  *a = xcos (*a);
  if (x_exp (b) < -80)
    *b = xZero;
  if (x_exp (a) < -80)
    *a = xZero;
}

cxpr
cxpwr (cxpr z, int n)
{
  xpr mod, arg;
  cxpr w;

  mod = cxabs (z);
  if (xsgn (&mod) <= 0)
    {
      int dummy;

      dummy = xsigerr (n <= 0, XEBADEXP, "cxpwr()");
      return cxZero;
    }
  else
    {
      arg = xmul (inttox (n), cxarg (z));
      mod = xpwr (mod, n);
      w.re = xmul (mod, xcos (arg));
      w.im = xmul (mod, xsin (arg));
      return w;
    }
  return w;
}

cxpr
cxroot (cxpr z, int i, int n)
{
  xpr mod, arg, e, a, b;
  cxpr w, zz;

  if (xsigerr (n == 0, XEBADEXP, "cxroot()"))
    return cxZero;
  else
    {
      mod = cxabs (z);
      if (xsgn (&mod) <= 0)
	{
	  int dummy;

	  dummy = xsigerr (n < 0, XEBADEXP, "cxroot()");
	  return cxZero;
	}
      else			/* mod > 0 */
	{
	  arg = xdiv (cxarg (z), inttox (n));
	  e = xdiv (xOne, inttox (n));	/* 1/n */
	  /* x^e = exp(e*log(x)) for any x > 0 */
	  mod = xexp (xmul (e, xlog (mod)));
	  w.re = xmul (mod, xcos (arg));
	  w.im = xmul (mod, xsin (arg));
	  unit_root (i, n, &a, &b);
	  zz.re = xadd (xmul (w.re, a), xmul (w.im, b), 1);
	  zz.im = xadd (xmul (w.im, a), xmul (w.re, b), 0);
	  return zz;
	}
    }
}

cxpr
cxpow (cxpr z1, cxpr z2)
{
  xpr mod, arg, a, b;
  cxpr w;

  mod = cxabs (z1);
  if (xsgn (&mod) <= 0)
    {
      int dummy;

      dummy = xsigerr (xsgn (&z2.re) <= 0, XEBADEXP, "cxpow()");
      return cxZero;
    }
  else
    {
      arg = cxarg (z1);
      a = xadd (xmul (z2.re, xlog (mod)), xmul (z2.im, arg), 1);
      b = xadd (xmul (z2.re, arg), xmul (z2.im, xlog (mod)), 0);
      w.re = xmul (xexp (a), xcos (b));
      w.im = xmul (xexp (a), xsin (b));
      return w;
    }
}
