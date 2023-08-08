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
cxadd (cxpr z1, cxpr z2, int k)
{
  cxpr w;

  w.re = xadd (z1.re, z2.re, k);
  w.im = xadd (z1.im, z2.im, k);
  return w;
}

cxpr
cxsum (cxpr z1, cxpr z2)
{
  cxpr w;

  w.re = xadd (z1.re, z2.re, 0);
  w.im = xadd (z1.im, z2.im, 0);
  return w;
}

cxpr
cxsub (cxpr z1, cxpr z2)
{
  cxpr w;

  w.re = xadd (z1.re, z2.re, 1);
  w.im = xadd (z1.im, z2.im, 1);
  return w;
}

cxpr
cxmul (cxpr z1, cxpr z2)
{
  cxpr w;

  w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 1);
  w.im = xadd (xmul (z1.im, z2.re), xmul (z1.re, z2.im), 0);
  return w;
}

/* Multiplication by a real number */
cxpr
cxrmul (xpr c, cxpr z)
{
  cxpr w;

  w.re = xmul (c, z.re);
  w.im = xmul (c, z.im);
  return w;
}

/* Multiplication by +i */
cxpr
cxdrot (cxpr z)
{
  cxpr y;

  y.re = z.im;
  y.im = z.re;
  y.re.nmm[0] ^= xM_sgn;
  return y;
/* MS-VC requires a more explicit coding,
   signaled on 2010-04-09 by A.Haumer

  z.im.nmm[0] ^= xM_sgn;
  return (cxpr)
  {
  z.im, z.re};
*/
}

/* Multiplication by -i */
cxpr
cxrrot (cxpr z)
{
  cxpr y;

  y.re = z.im;
  y.im = z.re;
  y.im.nmm[0] ^= xM_sgn;
  return y;
/* MS-VC requires a more explicit coding,
   signaled on 2010-04-09 by A.Haumer

  z.re.nmm[0] ^= xM_sgn;
  return (cxpr)
  {
  z.im, z.re};
*/
}

cxpr
cxdiv (cxpr z1, cxpr z2)
{
  int tv = cxrec (z2, &z2);

  if (!xsigerr (!tv, XEDIV, "cxdiv()"))
    {
      cxpr w;

      w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 1);
      w.im = xadd (xmul (z1.im, z2.re), xmul (z1.re, z2.im), 0);
      return w;
    }
  else
    return cxZero;
}

cxpr
cxinv (cxpr z)
{
  int tv = cxrec (z, &z);

  if (!xsigerr (!tv, XEDOM, "cxinv()"))
    return z;
  else
    return cxZero;
}

cxpr
cxsqr (cxpr z)
{
  cxpr w;

  w.re = xadd (xmul (z.re, z.re), xmul (z.im, z.im), 1);
  w.im = xmul (xTwo, xmul (z.im, z.re));
  return w;
}

cxpr
cxsqrt (cxpr z)
{
  xpr mod, arg;
  cxpr w;

  if (xsgn (&z.re) == 0 && xsgn (&z.im) == 0)
    return cxZero;
  else
    {
      mod = xsqrt (cxabs (z));
      arg = xpr2 (cxarg (z), -1);
      w.re = xmul (mod, xcos (arg));
      w.im = xmul (mod, xsin (arg));
      return w;
    }
}
