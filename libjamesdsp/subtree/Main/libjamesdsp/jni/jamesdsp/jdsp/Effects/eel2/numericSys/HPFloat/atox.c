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

#include<ctype.h>
#include "xpre.h"

#define UPPER_BOUND  (XMAX_10EX + 100)

xpr
strtox (const char *q, char **endptr)
{
  xpr s, f;
  unsigned short pc[XDIM + 1];
  unsigned short *pn, *pf, *pa, *pb;
  unsigned short sfg, ibex, fbex;
  unsigned long n;
  long j;
  int idex = 0, fdex = 0, c, m;
  short noip, nofp;
  const char *ptr;

  pn = (unsigned short *) &s;
  pf = (unsigned short *) &f;
  for (j = 0; j <= XDIM; pn[j] = pf[j] = pc[j] = 0, ++j);
  sfg = 0;
  m = XDIM + 1;
  if ((endptr))
    *endptr = (char *) q;
  /* Skip the leading spaces if there are some */
  while ((isspace (*q)))
    q++;
  /* Sign */
  if (*q == '+')
    ++q;
  else if (*q == '-')
    {
      sfg = 0x8000;
      ++q;
    }
  /* Integer part */
  for (ptr = q; (c = *q - '0') >= 0 && c <= 9; ++q)
    {
      if (pn[0])
	++idex;
      else
	{
	  xlshift (1, pn, m);
	  for (j = 0; j < m; ++j)
	    pc[j] = pn[j];
	  xlshift (2, pn, m);
	  for (n = (unsigned int) c, pa = pn + XDIM, pb = pc + XDIM;
	       pa >= pn; pa--, pb--)
	    {
	      n += *pa + *pb;
	      *pa = n;
	      n >>= 16;
	    }
	}
    }
  for (j = 0; j < m && pn[j] == 0; ++j);
  if (j == m)
    ibex = 0;
  else
    {
      ibex = xBias + xMax_p - 1;
      if (j)
	{
	  j <<= 4;
	  ibex -= j;
	  xlshift (j, pn, m);
	}
      while (pn[0])
	{
	  xrshift (1, pn, m);
	  ++ibex;
	}
      pn[0] = ibex | sfg;
    }
  noip = ptr == q;
  /* End Integer part */
  if (*q == '.')
    {
      /* Fractionary part */
      for (j = 0; j <= XDIM; ++j)
	pc[j] = 0;
      for (ptr = ++q; (c = *q - '0') >= 0 && c <= 9 && pf[0] == 0;
	   --fdex, ++q)
	{
	  xlshift (1, pf, m);
	  for (j = 0; j < m; ++j)
	    pc[j] = pf[j];
	  xlshift (2, pf, m);
	  for (n = (unsigned int) c, pa = pf + XDIM, pb = pc + XDIM;
	       pa >= pf; pa--, pb--)
	    {
	      n += *pa + *pb;
	      *pa = n;
	      n >>= 16;
	    }
	}
      for (j = 0; j < m && pf[j] == 0; ++j);
      if (j == m)
	fbex = 0;
      else
	{
	  fbex = xBias + xMax_p - 1;
	  if (j)
	    {
	      j <<= 4;
	      fbex -= j;
	      xlshift (j, pf, m);
	    }
	  while (pf[0])
	    {
	      xrshift (1, pf, m);
	      ++fbex;
	    }
	  pf[0] = fbex | sfg;
	}
      nofp = ptr == q;
    }				/* end if (*q == '.') */
  else
    nofp = 1;
  if ((noip) && (nofp))
    /* Error ! */
    return xNaN;
  else
    {
      /*
	Added on August 4th 2007:
	If the decimal digits after the radix character ('.')
	are too much with respect to the given precision,
	all the meaningless ones must be neglected.
	The absence of this loop produced a VERY hazardous bug
	(thanks to Shina Tan <tansn ~at~ phys ~dot~ washington ~dot~ edu>
	 for the bug notification)
      */
      for(; *q >= '0' && *q <= '9'; q++);
      /*
	End addition
      */
      if ((endptr))
	*endptr = (char *) q;
    }
  /* Exponent */
  if (*q == 'e' || *q == 'E')
    {
      ++q;
      sfg = 0;
      if (*q == '+')
	++q;
      else if (*q == '-')
	{
	  sfg = 1;
	  ++q;
	}
      for (ptr = q, j = 0; (c = *q - '0') >= 0 && c <= 9
	   && j <= UPPER_BOUND; ++q)
	{
	  j <<= 1;
	  m = j;
	  j <<= 2;
	  j += c + m;
	}
      if (ptr != q && (endptr))
	*endptr = (char *) q;
      if (sfg)
	j = -j;
      idex += j;
      fdex += j;
    }
  /*
    Remark: s and f have the same sign (see above).
  */
  if (idex > XMAX_10EX || fdex > XMAX_10EX)
    return ((s.nmm[0] & xM_sgn) ? xMinf : xPinf);
  else
    {
      if (idex)
	s = xmul (s, xpwr (xTen, idex));
      if (fdex)
	f = xmul (f, xpwr (xTen, fdex));
      return xadd (s, f, 0);
    }
}

xpr
atox (const char *q)
{
  return strtox (q, NULL);
}
