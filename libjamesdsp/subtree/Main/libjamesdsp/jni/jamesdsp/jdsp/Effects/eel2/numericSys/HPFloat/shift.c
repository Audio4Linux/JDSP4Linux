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

void
xlshift (int n, unsigned short *pm, int m)
{
  unsigned short *pa, *pc;

  pc = pm + m - 1;
  if (n < 16 * m)
    {
      pa = pm + n / 16;
      m = n % 16;
      n = 16 - m;
      while (pa < pc)
	{
	  *pm = (*pa++) << m;
	  *pm++ |= *pa >> n;
	}
      *pm++ = *pa << m;
    }
  while (pm <= pc)
    *pm++ = 0;
}

void
xrshift (int n, unsigned short *pm, int m)
{
  unsigned short *pa, *pc;

  pc = pm + m - 1;
  if (n < 16 * m)
    {
      pa = pc - n / 16;
      m = n % 16;
      n = 16 - m;
      while (pa > pm)
	{
	  *pc = (*pa--) >> m;
	  *pc-- |= *pa << n;
	}
      *pc-- = *pa >> m;
    }
  while (pc >= pm)
    *pc-- = 0;
}
