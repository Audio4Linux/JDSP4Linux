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

#include <float.h>
#include "xpre.h"

#define ABS(n) ((n) >= 0 ? (n) : -(n))

double
xtodbl (xpr s)
{
  unsigned short pe[4], *pc, u;
  short i, e;

  pc = (unsigned short *) &s;
  u = *pc & xM_sgn;
  e = (*pc & xM_exp) - xD_bias;
  if (e >= xD_max)
    return (!u ? DBL_MAX : -DBL_MAX);
  if (e <= 0)
    return 0.;
  for (i = 0; i < 4; pe[i] = *++pc, i++);
  pe[0] &= xM_exp;
  xrshift (xD_lex - 1, pe, 4);
  pe[0] |= (e << (16 - xD_lex));
  pe[0] |= u;
  /* Change made by Ivano Primi - 11/19/2004 */
  if (XLITTLE_ENDIAN)
  {
      u = pe[3];
      pe[3] = pe[0];
      pe[0] = u;
      u = pe[2];
      pe[2] = pe[1];
      pe[1] = u;
  }
  return *(double *) pe;
}

xpr
dbltox (double y)
{
  unsigned short pe[XDIM + 1], *pc, u;
  short i, e;

  if (y < DBL_MIN && y > -DBL_MIN)
    return xZero;
  pc = (unsigned short *) &y;
  /* Change made by Ivano Primi - 11/19/2004 */
  if (XLITTLE_ENDIAN)
    pc += 3;
  u = *pc & xM_sgn;
  e = xD_bias + ((*pc & xM_exp) >> (16 - xD_lex));
  /* Change made by Ivano Primi - 11/19/2004 */
  if (XLITTLE_ENDIAN)
      for (i = 1; i < 5; pe[i] = *pc--, i++);
  else
      for (i = 1; i < 5; pe[i] = *pc++, i++);
  while (i <= XDIM)
    pe[i++] = 0;
  pc = pe + 1;
  xlshift (xD_lex - 1, pc, 4);
  *pc |= xM_sgn;
  *pe = e;
  *pe |= u;
  return *(xpr *) pe;
}

/* Changed by Ivano Primi - 11/19/2004 */

xpr
inttox (long n)
{
  unsigned short pe[XDIM + 1], *pc;
  short e;
  unsigned long k, h;

  k = ABS (n);
  pc = (unsigned short *) &k;
  for (e = 0; e <= XDIM; pe[e++] = 0);
  if (n == 0)
    return *(xpr *) pe;

  if (XULONG_BITSIZE == 64)
  {
      if (XLITTLE_ENDIAN)
      {
          pe[1] = *(pc + 3);
          pe[2] = *(pc + 2);
          pe[3] = *(pc + 1);
          pe[4] = *pc;
      }
      else
      {
          pe[1] = *pc;
          pe[2] = *(pc + 1);
          pe[3] = *(pc + 2);
          pe[4] = *(pc + 3);
      }
  }
  else
  {
      if (XLITTLE_ENDIAN)
      {
          pe[1] = *(pc + 1);
          pe[2] = *pc;
      }
      else
      {
          pe[1] = *pc;
          pe[2] = *(pc + 1);
      }
  }

  for (e = 0, h = 1; h <= k && e < (XULONG_BITSIZE-1); h <<= 1, ++e);
  if (h <= k)
    e += 1;
  *pe = xBias + e - 1;
  if (n < 0)
    *pe |= xM_sgn;
  xlshift (XULONG_BITSIZE - e, pe + 1, XDIM);
  return *(xpr *) pe;
}

xpr
uinttox(unsigned long n)
{
    unsigned short pe[XDIM + 1], *pc;
    short e;
    unsigned long h;

    pc = (unsigned short *)&n;
    for (e = 0; e <= XDIM; pe[e++] = 0);
    if (n == 0)
        return *(xpr *)pe;

    if (XULONG_BITSIZE == 64)
    {
        if (XLITTLE_ENDIAN)
        {
            pe[1] = *(pc + 3);
            pe[2] = *(pc + 2);
            pe[3] = *(pc + 1);
            pe[4] = *pc;
        }
        else
        {
            pe[1] = *pc;
            pe[2] = *(pc + 1);
            pe[3] = *(pc + 2);
            pe[4] = *(pc + 3);
        }
    }
    else
    {
        if (XLITTLE_ENDIAN)
        {
            pe[1] = *(pc + 1);
            pe[2] = *pc;
        }
        else
        {
            pe[1] = *pc;
            pe[2] = *(pc + 1);
        }
    }

  for (e = 0, h = 1; h <= n && e < (XULONG_BITSIZE-1); h <<= 1, ++e);
  if (h <= n)
    e += 1;
  *pe = xBias + e - 1;
  xlshift (XULONG_BITSIZE - e, pe + 1, XDIM);
  return *(xpr *) pe;
}
