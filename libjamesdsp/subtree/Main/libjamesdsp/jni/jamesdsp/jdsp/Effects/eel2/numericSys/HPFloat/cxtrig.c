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
cxsin (cxpr z)
{
  cxpr w1, w2;

  w1 = cxdrot (z);		/* Now w1= i*z,  where i={0,1} */
  w2 = cxrrot (z);		/* Now w2= -i*z, where i={0,1} */
  w1 = cxsub (cxexp (w1), cxexp (w2));
  w2.re = xpr2 (w1.im, -1);
  w1.re.nmm[0] ^= xM_sgn;
  w2.im = xpr2 (w1.re, -1);	/* Now w2= (exp(i*z)-exp(-i*z))/2i */
  return w2;
}

cxpr
cxcos (cxpr z)
{
  cxpr w1, w2;

  w1 = cxdrot (z);		/* Now w1=  i*z,  where i={0,1} */
  w2 = cxrrot (z);		/* Now w2= -i*z, where i={0,1} */
  w1 = cxsum (cxexp (w1), cxexp (w2));
  w2.re = xpr2 (w1.re, -1);
  w2.im = xpr2 (w1.im, -1);
  return w2;
}

cxpr
cxtan (cxpr z)
{
  if (xsigerr (xprcmp (&z.im, &xEmax) > 0, XFPOFLOW, NULL))
    return cxIU;
  else if (xsigerr (xprcmp (&z.im, &xEmin) < 0, XFPOFLOW, NULL))
    return cxneg (cxIU);
  else
    {
      cxpr w;

      if (xsigerr (!cxrec (cxcos (z), &w), XEDOM, "cxtan()"))
	return cxZero;
      else
	return cxmul (cxsin (z), w);
    }
}

cxpr
cxasin (cxpr z)
{
  cxpr w;
  xpr ls, rs;

  w = cxsqrt (cxsub (cxOne, cxsqr (z)));
  ls = cxabs (cxsum (cxdrot (z), w));
  rs = xmul (xVSV, cxabs (z));
  if (xprcmp (&ls, &rs) < 0)
    w = cxdrot (cxlog (cxsub (w, cxdrot (z))));
  else
    w = cxrrot (cxlog (cxsum (cxdrot (z), w)));
  return w;
}

cxpr
cxacos (cxpr z)
{
  cxpr w;
  xpr ls, rs;

  w = cxsqrt (cxsub (cxOne, cxsqr (z)));
  ls = cxabs (cxsum (z, cxdrot (w)));
  rs = xmul (xVSV, cxabs (z));
  if (xprcmp (&ls, &rs) < 0)
    w = cxdrot (cxlog (cxsub (z, cxdrot (w))));
  else
    w = cxrrot (cxlog (cxsum (z, cxdrot (w))));
  return w;
}

cxpr
cxatan (cxpr z)
{
  cxpr w;
  xpr mod;
  int errcond;

  mod = xadd (xabs (z.im), xOne, 1);
  errcond = xsgn (&z.re) == 0 && xsgn (&mod) == 0;
  if (xsigerr (errcond, XEDOM, "cxatan()"))
    return cxZero;
  else
    {
      /* In this way, cxatan() works fine also with complex numbers */
      /* very far from the origin.                                  */
      mod = cxabs (z);
      if (xprcmp (&mod, &xVGV) > 0)
	{
	  w = cxsqrt (cxsum (cxOne, cxsqr (z)));
	  w = cxdiv (cxsum (cxOne, cxdrot (z)), w);
	  w = cxrrot (cxlog (w));
	}
      else
	{
	  w = cxdiv (cxsum (cxOne, cxdrot (z)), cxsub (cxOne, cxdrot (z)));
	  w = cxrrot (cxlog_sqrt (w));
	}
      return w;
    }
}
