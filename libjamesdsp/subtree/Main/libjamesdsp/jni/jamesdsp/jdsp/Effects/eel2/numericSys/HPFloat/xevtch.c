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
xevtch (xpr z, xpr *a, int m)
{
  xpr *p, f, t, tp, w;

  w = xpr2 (z, 1);
  t = xZero;
  tp = xZero;
  for (p = a + m; p > a;)
    {
      f = xadd (*p--, xadd (xmul (w, t), tp, 1), 0);
      tp = t;
      t = f;
    }
  return xadd (*p, xadd (xmul (z, t), tp, 1), 0);
}
