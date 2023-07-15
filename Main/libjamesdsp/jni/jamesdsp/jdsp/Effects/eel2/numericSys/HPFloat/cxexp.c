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
cxexp (cxpr z)
{
  cxpr w;

  w.re = xmul (xexp (z.re), xcos (z.im));
  w.im = xmul (xexp (z.re), xsin (z.im));
  return w;
}

cxpr
cxexp10 (cxpr z)
{
  cxpr w;

  z.im = xmul (xLn10, z.im);
  w.re = xmul (xexp10 (z.re), xcos (z.im));
  w.im = xmul (xexp10 (z.re), xsin (z.im));
  return w;
}

cxpr
cxexp2 (cxpr z)
{
  cxpr w;

  z.im = xmul (xLn2, z.im);
  w.re = xmul (xexp2 (z.re), xcos (z.im));
  w.im = xmul (xexp2 (z.re), xsin (z.im));
  return w;
}

cxpr
cxlog (cxpr z)
{
  xpr mod;
  cxpr w;

  mod = cxabs (z);
  if (xsigerr (xsgn (&mod) <= 0, XEDOM, "cxlog()"))
    return cxZero;
  else
    {
      w.re = xlog (mod);
      w.im = cxarg (z);
      return w;
    }
}

cxpr
cxlog10 (cxpr z)
{
  xpr mod;
  cxpr w;

  mod = cxabs (z);
  if (xsigerr (xsgn (&mod) <= 0, XEDOM, "cxlog10()"))
    return cxZero;
  else
    {
      w.re = xlog10 (mod);
      w.im = xmul (cxarg (z), xLog10_e);
      return w;
    }
}

cxpr
cxlog2 (cxpr z)
{
  xpr mod;
  cxpr w;

  mod = cxabs (z);
  if (xsigerr (xsgn (&mod) <= 0, XEDOM, "cxlog2()"))
    return cxZero;
  else
    {
      w.re = xlog2 (mod);
      w.im = xmul (cxarg (z), xLog2_e);
      return w;
    }
}

cxpr
cxlog_sqrt (cxpr z)
{
  xpr mod;
  cxpr w;

  mod = cxabs (z);
  if (xsigerr (xsgn (&mod) <= 0, XEDOM, "cxlog_sqrt()"))
    return cxZero;
  else
    {
      w.re = xpr2 (xlog (mod), -1);
      w.im = xpr2 (cxarg (z), -1);
      return w;
    }
}
