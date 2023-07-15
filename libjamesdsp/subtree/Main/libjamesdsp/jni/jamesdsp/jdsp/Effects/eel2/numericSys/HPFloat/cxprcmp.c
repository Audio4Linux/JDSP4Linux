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

cxprcmp_res
cxprcmp (const cxpr* z1, const cxpr* z2)
{
  cxprcmp_res r;

  r.re = xprcmp (&z1->re, &z2->re);
  r.im = xprcmp (&z1->im, &z2->im);
  return r;
}

int
cxis0 (const cxpr* z)
{
  return (xis0 (&z->re) && xis0 (&z->im));
}

int
cxnot0 (const cxpr* z)
{
  return (xnot0 (&z->re) || xnot0 (&z->im));
}

int
cxeq (cxpr z1, cxpr z2)
{
  return (xprcmp (&z1.re, &z2.re) == 0 && xprcmp (&z1.im, &z2.im) == 0);
}

int
cxneq (cxpr z1, cxpr z2)
{
  return (xprcmp (&z1.re, &z2.re) != 0 || xprcmp (&z1.im, &z2.im) != 0);
}

int
cxgt (cxpr z1, cxpr z2)
{
  int rs = xprcmp (&z1.re, &z2.re);
  int is = xprcmp (&z1.im, &z2.im);

  if (rs > 0)
    return (is >= 0);
  else if (is > 0)
    return (rs >= 0);
  else
    return 0;
}

int
cxge (cxpr z1, cxpr z2)
{
  return (xprcmp (&z1.re, &z2.re) >= 0 && xprcmp (&z1.im, &z2.im) >= 0);
}

int
cxlt (cxpr z1, cxpr z2)
{
  int rs = xprcmp (&z1.re, &z2.re);
  int is = xprcmp (&z1.im, &z2.im);

  if (rs < 0)
    return (is <= 0);
  else if (is < 0)
    return (rs <= 0);
  else
    return 0;
}

int
cxle (cxpr z1, cxpr z2)
{
  return (xprcmp (&z1.re, &z2.re) <= 0 && xprcmp (&z1.im, &z2.im) <= 0);
}
