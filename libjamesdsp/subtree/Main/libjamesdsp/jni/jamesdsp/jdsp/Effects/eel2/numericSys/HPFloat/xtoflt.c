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

#include <stdlib.h>
#include <float.h>
#include "xpre.h"

/*
       An extended floating point number is represented as a combination of the
 following elements:

                    sign bit(s): 0 -> positive, 1 -> negative ;
                    exponent(e): 15-bit biased integer (bias=16383) ;
                    mantissa(m): 7 (or more) words of 16 bit length with the
                                 leading 1 explicitly represented .

                   Thus  f = (-1)^s*2^[e-16383] *m ,  with 1 <= m < 2 .

 This format supports a dynamic range of:

                    2^16384 > f > 2^[-16383]  or

                    1.19*10^4932 > f > 1.68*10^-[4932].

 Special values of the exponent are:

                    all ones -> infinity (floating point overflow)
                    all zeros -> number = zero.

 Underflow in operations is handled by a flush to zero. Thus, a number with
 the exponent zero and nonzero mantissa is invalid (not-a-number).

*/

float
xtoflt (xpr s)
{
  unsigned short pe[2], *pc, u;
  short i, e;

  pc = (unsigned short *) &s;
  u = *pc & xM_sgn;
  e = (*pc & xM_exp) - xF_bias;
  /*
     u is the sign of the number s.
     e == (exponent of s) + 127 
   */
  if (e >= xF_max)
    return (!u ? FLT_MAX : -FLT_MAX);
  if (e <= 0)
    return 0.;
  for (i = 0; i < 2; pe[i] = *++pc, i++);

  /* In the IEEE 754 Standard the leading 1 */
  /* is not represented.                    */
  pe[0] &= xM_exp;
  /* Now in pe[0],pe[1] we have 31 bits of mantissa. */
  /* But only the first 23 ones must be put in the   */
  /* final float number.                             */
  xrshift (xF_lex - 1, pe, 2);
  /* We have just loaded the mantissa and now we */
  /* are going to load exponent and sign.        */
  pe[0] |= (e << (16 - xF_lex));
  pe[0] |= u;
  if (XLITTLE_ENDIAN)
  {
      u = pe[0];
      pe[0] = pe[1];
      pe[1] = u;
  }
  return *(float *) pe;
}

xpr
flttox (float y)
{
  unsigned short pe[XDIM + 1], *pc, u;
  short i, e;

  if (y < FLT_MIN && y > -FLT_MIN)
    return xZero;
  pc = (unsigned short *) &y;
  if (XLITTLE_ENDIAN)
      pc += 1;
  u = *pc & xM_sgn;
  e = xF_bias + ((*pc & xM_exp) >> (16 - xF_lex));
  /*
     Now u is the sign of y and e is the
     biased exponent (exponent + bias).
   */
  if (XLITTLE_ENDIAN)
      for (i = 1; i < 3; pe[i] = *pc--, i++);
  else
      for (i = 1; i < 3; pe[i] = *pc++, i++);
  while (i <= XDIM)
    pe[i++] = 0;
  pc = pe + 1;
  xlshift (xF_lex - 1, pc, 2);
  *pc |= xM_sgn;
  /* We have just put in pe[1],pe[2] the whole */
  /* mantissa of y with a leading 1.           */
  /* Now we have only to put exponent and sign */
  /* in pe[0].                                 */
  *pe = e;
  *pe |= u;
  return *(xpr *) pe;
}
