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
#include "xpre.h"

xpr*
xchcof (int m, xpr (*xfunc) (xpr))
{
  xpr a, b, *c, *cs;
  int i, j, k, n;

  if (m > XMAX_DEGREE)
    m = XMAX_DEGREE;
  ++m;
  n = 2 * m;
  if ( !(cs = (xpr*) malloc ((n+1)*sizeof(xpr))) )
    return NULL;
  else if ( !(c = (xpr*) malloc (m*sizeof(xpr))) )
    {
      free ((void*)cs);
      return NULL;
    }
  else
    {
      a = xdiv (xPi2, inttox (m));
      b = a;
      cs[0] = xOne;
      for (j = 0; j < m; ++j)
	c[j] = xZero;
      for (j = 1; j <= n; b = xadd (b, a, 0), ++j)
	cs[j] = xcos (b);
      for (j = 1; j < n; j += 2)
	{
	  a = (*xfunc) (cs[j]);
	  c[0] = xadd (c[0], a, 0);
	  for (k = 1; k < m; ++k)
	    {
	      i = (k * j) % (2 * n);
	      if (i > n)
		i = 2 * n - i;
	      c[k] = xadd (c[k], xmul (a, cs[i]), 0);
	    }
	}
      b = xpr2 (xdiv (xOne, inttox (m)), 1);
      for (j = 0; j < m; ++j)
	c[j] = xmul (c[j], b);
      free ((void*)cs);
      return c;
    }
}
