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

#include<stdlib.h>		/* for free() */
#include<string.h>		/* for strcat() */
#include"cxpre.h"		/* Automatically includes "xpre.h" */

/* See print.c for the next 2 functions */
extern void xfileputc (int c, FILE * stream);
extern void xprintfmt (FILE * stream, const char *fmt, ...);

void
cxpr_print (FILE * stream, cxpr z, int sc_not, int sign, int lim)
{
  xpr_print (stream, z.re, sc_not, sign, lim);
  xpr_print (stream, z.im, sc_not, 1, lim);
  xfileputc (CX1I_CHAR, stream);
}

void
cxprcxpr (cxpr z, int lim)
{
  cxpr_print (stdout, z, 1, 0, lim);
}

void
cxprint (FILE * stream, cxpr z)
{
  int i;

  xprintfmt (stream, "(%04x.", z.re.nmm[0]);
  for (i = 0; i < XDIM; ++i)
    xprintfmt (stream, "%04x", z.re.nmm[i + 1]);
  xprintfmt (stream, " , %04x.", z.im.nmm[0]);
  for (i = 0; i < XDIM; ++i)
    xprintfmt (stream, "%04x", z.im.nmm[i + 1]);
  xprintfmt (stream, ")\n");
}

int
cxfout (FILE * fp, xoutflags ofs, cxpr z)
{
  if (ofs.fmt < 0)
    ofs.fmt = XFMT_STD;
  else if (ofs.fmt > 2)
    ofs.fmt = XFMT_ALT;
  if (ofs.fmt == XFMT_ALT)
    {
      if (ofs.ldel < 0)
	ofs.ldel = CXDEF_LDEL;
      if (ofs.rdel < 0)
	ofs.rdel = CXDEF_RDEL;
      if (fputc (ofs.ldel, fp) == EOF)
	return -1;
    }
  if ((xfout (fp, ofs, z.re)))
    return -1;
  else if (ofs.fmt == XFMT_RAW && fprintf (fp, CX_EMPTY_SEP) < 0)
    return -1;
  else if (ofs.fmt == XFMT_ALT && fprintf (fp, CX_SEPARATOR) < 0)
    return -1;
  else if ((ofs.sf = ofs.fmt == XFMT_STD ? 1 : ofs.sf, xfout (fp, ofs, z.im)))
    return -1;
  else
    {
      if (ofs.fmt == XFMT_STD)
	{
	  if (fputc (CX1I_CHAR, fp) == EOF)
	    return -1;
	}
      else if (ofs.fmt == XFMT_ALT)
	{
	  if (fputc (ofs.rdel, fp) == EOF)
	    return -1;
	}
      return 0;
    }
}

/* Exactly the same as xfout(), but it prints on stdout */

int
cxout (xoutflags ofs, cxpr z)
{
  return cxfout (stdout, ofs, z);
}

unsigned long
cxsout (char *s, unsigned long n, xoutflags ofs, cxpr z)
{
  char *ptr;
  unsigned long u, retval;

  if (!s || n == 0)
    return 0;			/* Just as xsout() under the same condition */
  else
    ptr = s, retval = 0;
  /* Remark: we are sure that n >= 1 */
  if (ofs.fmt < 0)
    ofs.fmt = XFMT_STD;
  else if (ofs.fmt > 2)
    ofs.fmt = XFMT_ALT;
  /* Optional left delimiter */
  if (ofs.fmt == XFMT_ALT)
    {
      if (ofs.ldel < 0)
	ofs.ldel = CXDEF_LDEL;
      if (ofs.rdel < 0)
	ofs.rdel = CXDEF_RDEL;
      if (n > 1)
	*ptr++ = (char) ofs.ldel, n--;
      retval++;
    }
  /* Real part */
  if ((u = xsout (ptr, n, ofs, z.re)) < n)
    ptr += u, n -= u;
  retval += u;
  /* Separator */
  if (ofs.fmt != XFMT_STD)
    {
      if (n > CX_SEP_L)
	{
	  if (ofs.fmt == XFMT_ALT)
	    strcat (ptr, CX_SEPARATOR);
	  else
	    strcat (ptr, CX_EMPTY_SEP);	/* RAW format enabled */
	  ptr += CX_SEP_L, n -= CX_SEP_L;
	}
      retval += CX_SEP_L;
    }
  /* Imaginary part */
  if ((ofs.sf = ofs.fmt == XFMT_STD ? 1 : ofs.sf,
       u = xsout (ptr, n, ofs, z.im)) < n)
    ptr += u, n -= u;
  retval += u;
  /* Optional right delimiter */
  if (ofs.fmt == XFMT_STD)
    {
      if (n > 1)
	*ptr++ = (char) CX1I_CHAR, n--;
      retval += 1;
    }
  else if (ofs.fmt == XFMT_ALT)
    {
      if (n > 1)
	*ptr++ = (char) ofs.rdel, n--;
      retval += 1;
    }
  *ptr = '\0';			/* We have to close the string ! */
  return retval;
}
