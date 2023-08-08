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
xneg (xpr s)
{
  unsigned short *p = (unsigned short *) &s;

  *p ^= xM_sgn;
  return s;
}

xpr
xabs (xpr s)
{
  unsigned short *p = (unsigned short *) &s;

  *p &= xM_exp;
  return s;
}

int
x_exp (const xpr *ps)
{
  unsigned short *q = (unsigned short *) ps;

  return (*q & xM_exp) - xBias;
}

int
x_neg (const xpr *ps)
{
  unsigned short *q = (unsigned short *) ps;

  return (*q & xM_sgn);
}
