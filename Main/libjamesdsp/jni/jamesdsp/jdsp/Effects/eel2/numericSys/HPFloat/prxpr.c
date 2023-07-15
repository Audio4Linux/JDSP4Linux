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

#include <stdlib.h>		/* for calloc() */
#include <string.h>             /* for strcpy() */
#include "xpre.h"		/* Automatically includes <stdio.h> */

const double __Log10_2__ = .3010299956639812;

/* Source code added by Ivano Primi - 11/28/2004 */

/* See print.c for all these functions */
extern void xfileputc (int c, FILE * stream);
extern void xstrputc (char c, char* buffer);
extern void xprintfmt (FILE * stream, const char *fmt, ...);
extern void xsprintfmt (char *buffer, const char *fmt, ...);

static int
printed_special_value (FILE* stream, xpr u, int sign)
{
  if ((xisPinf(&u)))
    {
      if ((sign))
	xfileputc ('+', stream);
      xprintfmt (stream, "Inf");
      return 1;
    }
  else if ((xisMinf(&u)))
    {
      xprintfmt (stream, "-Inf");
      return 1;
    }
  else if ((xisNaN(&u)))
    {
      if ((sign))
	xfileputc ('\?', stream);
      xprintfmt (stream, "NaN");
      return 1;
    }
  else
    return 0;
}


/*
  Remark: 's' must have a size >= 5 !
*/

static int
copied_special_value (char* s, xpr u, int sign)
{
  if ((xisPinf(&u)))
    {
      if ((sign))
	*s = '+', s++;
      strcpy (s, "Inf");
      return 1;
    }
  else if ((xisMinf(&u)))
    {
      strcpy (s, "-Inf");
      return 1;
    }
  else if ((xisNaN(&u)))
    {
      if ((sign))
	*s = '\?', s++;
      strcpy (s, "NaN");
      return 1;
    }
  else
    return 0;
}

/* sc_not != 0 tells using scientific notation                     */
/* sign   != 0 tells putting a '+' sign before non negative values */

void
xpr_print (FILE * stream, xpr u, int sc_not, int sign, int lim)
{
  char q[5 * XDIM + 4];
  register char *p = q;
  register int k, m;
  int dig;
  unsigned short *pa = (unsigned short *) &u;

  if (lim < 0)
    lim = 0;
  if (lim >= 5 * XDIM + 2)
    lim = 5 * XDIM + 2;
  if ((printed_special_value (stream, u, sign)))
    return;
  if ((*pa & xM_sgn))
    {
      *pa ^= xM_sgn;
      xfileputc ('-', stream);
    }
  else
    {
      if ((sign))
	xfileputc ('+', stream);
    }
  if ((xis0(&u)))
    {
      xprintfmt (stream, "0.");
      for (k = 0; k < lim; ++k)
	xfileputc ('0', stream);
      if ((sc_not))
	xprintfmt (stream, "e+0");
    }
  else
    {
      m = ((*pa & xM_exp) - xBias);
      m = (int) ((double) (m + 1) * __Log10_2__);
      if ((m))
	u = xmul (u, xpwr (xTen, -m));
      while ((*pa & xM_exp) < xBias)
	{
	  --m;
	  u = xmul (u, xTen);
	}
      for (*p = 0, k = 0; k <= lim; ++k)
	{
	  u = xsfmod (u, &dig);
	  ++p, *p = (char) dig;
	  if (*pa == 0)
	    break;
	  u = xmul (xTen, u);
	}
      for (; k <= lim; ++k)
	*++p = 0;
      if ((*pa))
	{
	  u = xsfmod (u, &dig);
	  if (dig >= 5)
	    ++(*p);
	  while (*p == 10)
	    {
	      *p = 0;
	      ++(*--p);
	    }
	}
      p = q;
      if (*p == 0)
	++p;
      else
	++m;
      /* Now has come the moment to print */
      if (m > XMAX_10EX)
	xprintfmt (stream, "Inf");
      else if ((sc_not))
	{
	  xprintfmt (stream, "%c.", '0' + *p++);
	  for (k = 0; k < lim; ++k)
	    xfileputc ('0'+ *p++, stream);
	  if (m >= 0)
	    xprintfmt (stream, "e+%d", m);
	  else
	    xprintfmt (stream, "e%d", m);
	}
      else
	{
	  if (m >= 0)
	    {
	      for (k = 0; k <= m; k++)
		{
		  if (k <= lim)
		    xfileputc ('0' + p[k], stream);
		  else
		    xfileputc ('0', stream);
		}
	      if (k <= lim)
		{
		  xfileputc ('.', stream);
		  for (; k <= lim; k++)
		    xfileputc ('0' + p[k], stream);
		}
	    }
	  else
	    {
	      xprintfmt (stream, "0.");
	      for (k = 1; k < -m; k++)
		xfileputc ('0', stream);
	      for (k = 0; k <= lim; ++k)
		xfileputc ('0' + *p++, stream);
	    }
	}
    }				/* End of *pa != 0 */
}

#define BUFF_SIZE 5120		/* 5 Kb !!! */

char *
xpr_asprint (xpr u, int sc_not, int sign, int lim)
{
  char q[5 * XDIM + 4], *buffer, *ptr;
  register char *p = q;
  register int k, m;
  int dig;
  unsigned short *pa = (unsigned short *) &u;

  if (lim < 0)
    lim = 0;
  if (lim > 5 * XDIM + 2)
    lim = 5 * XDIM + 2;
  if (!(buffer = (char *) calloc (BUFF_SIZE, sizeof (char))))
    return NULL;
  else if((copied_special_value (buffer, u, sign)))
    {
      for (k = 0; buffer[k] != '\0'; k++);
      /* Now k is the length of the buffer. */
      /* We shrink the buffer so that it has the exact */
      /* size to contain all its non null chars.       */
      ptr = (char *) realloc (buffer, k + 1);
      return (ptr != NULL) ? ptr : buffer;
    }
  else
    {
      if ((*pa & xM_sgn))
	{
	  *pa ^= xM_sgn;
	  xstrputc ('-', buffer);
	}
      else
	{
	  if ((sign))
	  xstrputc ('+', buffer);
	}
      if ((xis0(&u)))
	{
	  xsprintfmt (buffer, "0.");
	  for (k = 0; k < lim; ++k)
	    xstrputc ('0', buffer);
	  if ((sc_not))
	    xsprintfmt (buffer, "e+0");
	}
      else
	{
	  m = ((*pa & xM_exp) - xBias);
	  m = (int) ((double) (m + 1) * __Log10_2__);
	  if ((m))
	    u = xmul (u, xpwr (xTen, -m));
	  while ((*pa & xM_exp) < xBias)
	    {
	      --m;
	      u = xmul (u, xTen);
	    }
	  for (*p = 0, k = 0; k <= lim; ++k)
	    {
	      u = xsfmod (u, &dig);
	      ++p, *p = (char) dig;
	      if (*pa == 0)
		break;
	      u = xmul (xTen, u);
	    }
	  for (; k <= lim; ++k)
	    *++p = 0;
	  if ((*pa))
	    {
	      u = xsfmod (u, &dig);
	      if (dig >= 5)
		++(*p);
	      while (*p == 10)
		{
		  *p = 0;
		  ++(*--p);
		}
	    }
	  p = q;
	  if (*p == 0)
	    ++p;
	  else
	    ++m;
	  /* Now has come the moment to print */
	  if (m > XMAX_10EX)
	    xsprintfmt (buffer, "Inf");
	  else if ((sc_not))
	    {
	      xsprintfmt (buffer, "%c.", '0' + *p++);
	      for (k = 0; k < lim; ++k)
		xstrputc ('0' + *p++, buffer);
	      if (m >= 0)
		xsprintfmt (buffer, "e+%d", m);
	      else
		xsprintfmt (buffer, "e%d", m);
	    }
	  else
	    {
	      if (m >= 0)
		{
		  for (k = 0; k <= m; k++)
		    {
		      if (k <= lim)
			xstrputc ('0' + p[k], buffer);
		      else
			xstrputc ('0', buffer);
		    }
		  if (k <= lim)
		    {
		      xstrputc ('.', buffer);
		      for (; k <= lim; k++)
			xstrputc ('0' + p[k], buffer);
		    }
		}
	      else
		{
		  xsprintfmt (buffer, "0.");
		  for (k = 1; k < -m; k++)
		    xstrputc ('0', buffer);
		  for (k = 0; k <= lim; ++k)
		    xstrputc ('0' + *p++, buffer);
		}
	    }
	}			/* End of *pa != 0 */
      for (k = 0; buffer[k] != '\0'; k++);
      /* Now k is the length of the buffer. */
      /* We shrink the buffer so that it has the exact */
      /* size to contain all its non null chars.       */
      ptr = (char *) realloc (buffer, k + 1);
      return (ptr != NULL) ? ptr : buffer;
    }				/* End of buffer != 0 */
}

char *
xtoa (xpr u, int lim)
{
  return xpr_asprint (u, 1, 0, lim);
}

void
xbprint (FILE * stream, xpr u)
{
  register int i;
  register unsigned short n;

  for (n = 0x8000; n != 0x0; n >>= 1)
    {
      if ((n & u.nmm[0]))
	xfileputc ('1', stream);
      else
	xfileputc ('0', stream);
    }
  xfileputc ('.', stream);
  for (i = 0; i < XDIM; ++i)
    {
      for (n = 0x8000; n != 0x0; n >>= 1)
	{
	  if ((n & u.nmm[i + 1]))
	    xfileputc ('1', stream);
	  else
	    xfileputc ('0', stream);
	}
      xfileputc (' ', stream);
    }
  xfileputc ('\n', stream);
}

/* End Additions 11/28/2004 */

void
xprxpr (xpr u, int lim)
{
  /* Modified by Ivano Primi - 11/29/2004 */
  xpr_print (stdout, u, 1, 0, lim);
}

void
xprint (FILE * stream, xpr u)
{
  register int i;

  /* Modified by Ivano Primi - 4/2/2005 */
  xprintfmt (stream, "%04x.", u.nmm[0]);
  for (i = 0; i < XDIM; ++i)
    xprintfmt (stream, "%04x", u.nmm[i + 1]);
  xprintfmt (stream, "\n");
}

/* 
   Special output functions. 
   Added by Ivano Primi, 01/06/2005 
*/

/* See print.c for the next three functions */

extern int xwprint (const char *buff, short mfwd, char padding, FILE * fp);
extern void xwsprint (char *dest, const char *src, short mfwd, char padding);
extern unsigned short xwsnprint (char *dest, size_t dsize, const char *src,
				 short mfwd, char padding);

int
xfout (FILE * fp, xoutflags ofs, xpr x)
{
  char *str;
  int errcode;

  ofs.notat = (ofs.notat <= 0) ? XOUT_FIXED : XOUT_SCIENTIFIC;
  ofs.sf = (ofs.sf <= 0) ? 0 : 1;
  if (ofs.lim < 0)
    ofs.lim = XDEF_LIM;
  if (ofs.padding < 0)
    ofs.padding = ' ';		/* blank */
  str = xpr_asprint (x, ofs.notat, ofs.sf, ofs.lim);
  errcode = xwprint (str, ofs.mfwd, ofs.padding, fp);
  if ((str))
    free ((void *) str);
  return errcode;
}

/* Exactly the same as xfout(), but it prints on stdout */

int
xout (xoutflags ofs, xpr x)
{
  char *str;
  int errcode;

  ofs.notat = (ofs.notat <= 0) ? XOUT_FIXED : XOUT_SCIENTIFIC;
  ofs.sf = (ofs.sf <= 0) ? 0 : 1;
  if (ofs.lim < 0)
    ofs.lim = XDEF_LIM;
  if (ofs.padding < 0)
    ofs.padding = ' ';		/* blank */
  str = xpr_asprint (x, ofs.notat, ofs.sf, ofs.lim);
  errcode = xwprint (str, ofs.mfwd, ofs.padding, stdout);
  if ((str))
    free ((void *) str);
  return errcode;
}

/*
  Remark: xsout() returns an 'unsigned short' value
  casted to 'unsigned long'.
*/

unsigned long
xsout (char *s, unsigned long n, xoutflags ofs, xpr x)
{
  char *str;
  unsigned long nw;

  ofs.notat = (ofs.notat <= 0) ? XOUT_FIXED : XOUT_SCIENTIFIC;
  ofs.sf = (ofs.sf <= 0) ? 0 : 1;
  if (ofs.lim < 0)
    ofs.lim = XDEF_LIM;
  if (ofs.padding < 0)
    ofs.padding = ' ';		/* blank */
  str = xpr_asprint (x, ofs.notat, ofs.sf, ofs.lim);
  nw = xwsnprint (s, n, str, ofs.mfwd, ofs.padding);
  if ((str))
    free ((void *) str);
  return nw;
}
