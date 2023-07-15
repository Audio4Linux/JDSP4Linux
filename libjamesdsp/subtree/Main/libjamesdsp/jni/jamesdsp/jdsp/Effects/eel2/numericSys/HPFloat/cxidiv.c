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
cxgdiv (cxpr z1, cxpr z2)
{
  xpr mod2;

  z1.re = xround (z1.re);
  z1.im = xround (z1.im);
  z2.re = xround (z2.re);
  z2.im = xround (z2.im);
  mod2 = xadd (xmul (z2.re, z2.re), xmul (z2.im, z2.im), 0);
  if (xsigerr (xprcmp (&mod2, &xPinf) >= 0, XFPOFLOW, NULL) ||
      xsigerr (xsgn (&mod2) <= 0, XEDIV, "cxgdiv()"))
    return cxZero;
  else
    {
      cxpr w;

      w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 0);
      w.im = xadd (xmul (z2.re, z1.im), xmul (z2.im, z1.re), 1);
      w.re = xround (xdiv (w.re, mod2));
      w.im = xround (xdiv (w.im, mod2));
      return w;
    }
}

cxpr
cxidiv (cxpr z1, cxpr z2)
{
  xpr mod2;

  z1.re = xround (z1.re);
  z1.im = xround (z1.im);
  z2.re = xround (z2.re);
  z2.im = xround (z2.im);
  mod2 = xadd (xmul (z2.re, z2.re), xmul (z2.im, z2.im), 0);
  if (xsigerr (xprcmp (&mod2, &xPinf) >= 0, XFPOFLOW, NULL) ||
      xsigerr (xsgn (&mod2) <= 0, XEDIV, "cxidiv()"))
    return cxZero;
  else
    {
      cxpr w;

      w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 0);
      w.im = xadd (xmul (z2.re, z1.im), xmul (z2.im, z1.re), 1);
      w.re = xfix (xdiv (w.re, mod2));
      w.im = xfix (xdiv (w.im, mod2));
      return w;
    }
}

cxpr
cxgmod (cxpr z1, cxpr z2)
{
  xpr mod2;

  z1.re = xround (z1.re);
  z1.im = xround (z1.im);
  z2.re = xround (z2.re);
  z2.im = xround (z2.im);
  mod2 = xadd (xmul (z2.re, z2.re), xmul (z2.im, z2.im), 0);
  if (xsigerr (xprcmp (&mod2, &xPinf) >= 0, XFPOFLOW, NULL) ||
      xsigerr (xsgn (&mod2) <= 0, XEDIV, "cxgmod()"))
    return cxZero;
  else
    {
      cxpr w, z;

      w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 0);
      w.im = xadd (xmul (z2.re, z1.im), xmul (z2.im, z1.re), 1);
      w.re = xround (xdiv (w.re, mod2));
      w.im = xround (xdiv (w.im, mod2));
      z.re = xadd (xmul (w.re, z2.re), xmul (w.im, z2.im), 1);
      z.im = xadd (xmul (w.im, z2.re), xmul (w.re, z2.im), 0);
      w.re = xround (xadd (z1.re, z.re, 1));
      w.im = xround (xadd (z1.im, z.im, 1));
      return w;
    }
}

cxpr
cxmod (cxpr z1, cxpr z2)
{
  xpr mod2;

  z1.re = xround (z1.re);
  z1.im = xround (z1.im);
  z2.re = xround (z2.re);
  z2.im = xround (z2.im);
  mod2 = xadd (xmul (z2.re, z2.re), xmul (z2.im, z2.im), 0);
  if (xsigerr (xprcmp (&mod2, &xPinf) >= 0, XFPOFLOW, NULL) ||
      xsigerr (xsgn (&mod2) <= 0, XEDIV, "cxmod()"))
    return cxZero;
  else
    {
      cxpr w, z;

      w.re = xadd (xmul (z1.re, z2.re), xmul (z1.im, z2.im), 0);
      w.im = xadd (xmul (z2.re, z1.im), xmul (z2.im, z1.re), 1);
      w.re = xfix (xdiv (w.re, mod2));
      w.im = xfix (xdiv (w.im, mod2));
      z.re = xadd (xmul (w.re, z2.re), xmul (w.im, z2.im), 1);
      z.im = xadd (xmul (w.im, z2.re), xmul (w.re, z2.im), 0);
      w.re = xround (xadd (z1.re, z.re, 1));
      w.im = xround (xadd (z1.im, z.im, 1));
      return w;
    }
}
