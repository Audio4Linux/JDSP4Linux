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

/*
  renamed ctan to c_tan, suggested on 2010-04-09 by A.Haumer
*/
static xpr c_tan (xpr u);
static xpr rred (xpr u, int i, int *p);

/* Added by Ivano Primi - 01/30/2005 */

static int
xodd (xpr x)
{
  unsigned short *p = (unsigned short *) &x;
  short e, i;

  e = (*p & xM_exp) - xBias;	/* exponent of x */
  if (e < 0)
    return 0;
  else
    {
      for (i = 1; e / 16 > 0; i++, e -= 16);
      /* Now e = 0, ..., 15 */
      return (i <= XDIM ? p[i] & 0x8000 >> e : 0);
    }
}

xpr
xtan (xpr z)
{
  int k, m;

  z = rred (z, 't', &k);
  if ((xsigerr (xprcmp (&z, &xPi2) >= 0, XEDOM, "xtan()")))
    return (!k ? xPinf : xMinf);
  else
    {
      if (xprcmp (&z, &xPi4) == 1)
	{
	  m = 1;
	  z = xadd (xPi2, z, 1);
	}
      else
	m = 0;
      if ((k))
	z = xneg (c_tan (z));
      else
	z = c_tan (z);
      if (m)
	return xdiv (xOne, z);
      else
	return z;
    }
}

xpr
xcos (xpr z)
{
  int k;

  z = rred (z, 'c', &k);
  if (x_exp (&z) < xK_lin)
    {
      if ((k))
	return xneg (xOne);
      else
	return xOne;
    }
  z = c_tan (xpr2 (z, -1));
  z = xmul (z, z);
  z = xdiv (xadd (xOne, z, 1), xadd (xOne, z, 0));
  if ((k))
    return xneg (z);
  else
    return z;
}

xpr
xsin (xpr z)
{
  int k;

  z = rred (z, 's', &k);
  if (x_exp (&z) >= xK_lin)
    {
      z = c_tan (xpr2 (z, -1));
      z = xdiv (xpr2 (z, 1), xadd (xOne, xmul (z, z), 0));
    }
  if ((k))
    return xneg (z);
  else
    return z;
}

static xpr
c_tan (xpr z)
{
  xpr s, f, d;
  int m;
  unsigned short k;

  if (x_exp (&z) < xK_lin)
    return z;
  s = xneg (xmul (z, z));
  for (k = 1; k <= XDIM && s.nmm[k] == 0; k++);
  if ((xsigerr (s.nmm[0] == 0xffff && k > XDIM, XFPOFLOW, NULL)))
    return xZero;
  else
    {
      f = xZero;
      for (d = inttox (m = xMS_trg); m > 1;)
	{
	  f = xdiv (s, xadd (d, f, 0));
	  d = inttox (m -= 2);
	}
      return xdiv (z, xadd (d, f, 0));
    }
}

static xpr
rred (xpr z, int kf, int *ps)
{
  xpr is, q;

  if (x_neg (&z))
    {
      z = xneg (z);
      is = xOne;
    }
  else
    is = xZero;
  z = xfmod (z, xPi, &q);
  if (kf == 't')
    q = is;
  else if (kf == 's')
    q = xadd (q, is, 0);
  if (xprcmp (&z, &xPi2) == 1)
    {
      z = xadd (xPi, z, 1);
      if (kf == 'c' || kf == 't')
	q = xadd (q, xOne, 0);
    }
  *ps = (xodd (q)) ? 1 : 0;
  return z;
}
