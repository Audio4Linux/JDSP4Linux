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

#include<stdio.h>
#include<stdarg.h>
#include<string.h>
#include<stddef.h>		/* for size_t */
#include "xpre.h"

void
xfileputc (int c, FILE * stream)
{
  if (fputc (c, stream) == EOF)
    {
      fprintf (stderr, "*** In file  \"%s\" of the HPA library\n", __FILE__);
      fprintf (stderr, "   xfileputc(): I/O Error on stream %p\n", stream);
    }
}

void
xstrputc (char c, char* buffer)
{
  register char *ptr;

  for (ptr = buffer; *ptr != '\0'; ptr++);
  *ptr = c;
}

void
xprintfmt (FILE * stream, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  if (vfprintf (stream, fmt, ap) < 0)
    {
      fprintf (stderr, "*** In file  \"%s\" of the HPA library\n", __FILE__);
      fprintf (stderr, "   xprintfmt(): I/O Error on stream %p\n", stream);
    }
  va_end (ap);
}

/* Be really careful to the way you employ this function ! */

void
xsprintfmt (char *buffer, const char *fmt, ...)
{
  char ibuff[1024];
  va_list ap;

  va_start (ap, fmt);
  vsprintf (ibuff, fmt, ap);
  va_end (ap);
  strcat (buffer, ibuff);
}

int
xwprint (const char *buff, short mfwd, char padding, FILE * fp)
{
  unsigned short length = strlen (buff);
  register long i;

  if (!buff || !fp)
    return -1;
  else if (mfwd < 0)
    {
      if (fprintf (fp, "%s", buff) < 0)
	return -1;
      else
	{
	  for (i = length; i < -mfwd; i++)
	    if (fputc (padding, fp) == EOF)
	      return -1;
	  return 0;
	}
    }
  else
    {
      for (i = length; i < mfwd; i++)
	if (fputc (padding, fp) == EOF)
	  return -1;
      if (fprintf (fp, "%s", buff) < 0)
	return -1;
      else
	return 0;
    }
}

void
xwsprint (char *dest, const char *src, short mfwd, char padding)
{
  unsigned short length = strlen (src);
  register long i;

  if (!dest || !src)
    return;
  else if (mfwd < 0)
    {
      strcpy (dest, src);
      for (i = length; i < -mfwd; i++)
	dest[i] = padding;
      dest[i] = '\0';
    }
  else
    {
      for (i = length; i < mfwd; i++)
	dest[i - length] = padding;
      dest[i - length] = '\0';
      strcat (dest, src);
    }
}

unsigned short
xwsnprint (char *dest, size_t dsize, const char *src,
	   short mfwd, char padding)
{
  unsigned short n = strlen (src);
  register long i;

  if (dsize == 0 || !dest || !src)
    return 0;
  else if (mfwd < 0)
    {
      unsigned short l = n;

      strncpy (dest, src, dsize - 1);
      if (n >= dsize)
	n = dsize - 1;
      /* n is the number of chars copied */
      /* from 'src' to 'dest'.           */
      for (i = n; i < dsize - 1 && i < -mfwd; i++)
	dest[i] = padding;
      dest[i] = '\0';
      return (-mfwd > l ? -mfwd : l);
    }
  else
    {
      for (i = n; i - n < dsize - 1 && i < mfwd; i++)
	dest[i - n] = padding;
      dest[i - n] = '\0';
      if (i - n < dsize - 1)
	strncat (dest, src, dsize - 1 + n - i);
      dest[dsize - 1] = '\0';
      return (mfwd > n ? mfwd : n);
    }
}
