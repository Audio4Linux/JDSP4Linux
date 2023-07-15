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
cxsinh (cxpr z)
{
  cxpr w;

  w = cxsub (cxexp (z), cxexp (cxneg (z)));
  w.re = xpr2 (w.re, -1);
  w.im = xpr2 (w.im, -1);
  return w;
}

cxpr
cxcosh (cxpr z)
{
  cxpr w;

  w = cxsum (cxexp (z), cxexp (cxneg (z)));
  w.re = xpr2 (w.re, -1);
  w.im = xpr2 (w.im, -1);
  return w;
}

cxpr
cxtanh (cxpr z)
{
  if (xsigerr (xprcmp (&z.re, &xEmax) > 0, XFPOFLOW, NULL))
    return cxOne;
  else if (xsigerr (xprcmp (&z.re, &xEmin) < 0, XFPOFLOW, NULL))
    return cxneg (cxOne);
  else
    {
      cxpr w;

      if (xsigerr (!cxrec (cxcosh (z), &w), XEDOM, "cxtanh()"))
	return cxZero;
      else
	return cxmul (cxsinh (z), w);
    }
}

cxpr
cxasinh (cxpr z)
{
  cxpr w;
  xpr ls, rs;

  /* In this way, cxasinh() works fine also with real numbers */
  /* very near to -oo.                                       */
  w = cxsqrt (cxsum (cxOne, cxsqr (z)));
  ls = cxabs (cxsum (z, w));
  rs = xmul (xVSV, cxabs (z));
  if (xprcmp (&ls, &rs) < 0)
    return cxneg (cxlog (cxsub (w, z)));
  else
    return cxlog (cxsum (z, w));
}

cxpr
cxacosh (cxpr z)
{
  cxpr w;
  xpr ls, rs;

  w = cxsqrt (cxsub (cxsqr (z), cxOne));
  ls = cxabs (cxsum (z, w));
  rs = xmul (xVSV, cxabs (z));
  if (xprcmp (&ls, &rs) < 0)
    return cxneg (cxlog (cxsub (z, w)));
  else
    return cxlog (cxsum (z, w));
}

cxpr
cxatanh (cxpr z)
{
  cxpr w;
  xpr t;
  int errcond;

  t = xadd (xabs (z.re), xOne, 1);
  errcond = xsgn (&z.im) == 0 && xsgn (&t) == 0;
  if (xsigerr (errcond, XEDOM, "cxatanh()"))
    return cxZero;
  else
    {
      w = cxdiv (cxsum (cxOne, z), cxsub (cxOne, z));
      w = cxlog_sqrt (w);
      return w;
    }
}
