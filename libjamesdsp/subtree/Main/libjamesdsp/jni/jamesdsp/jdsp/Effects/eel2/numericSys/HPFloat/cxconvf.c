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

#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include"cxpre.h"		/* Automatically includes "xpre.h" */

cxpr
strtocx (const char *q, char **endptr)
{
  cxpr z = cxZero;
  char *ptr, *ptr2;

  z.re = strtox (q, &ptr);
  if ((endptr))
    *endptr = ptr;
  if (ptr != q)
    {
      if (*ptr == CX1I_CHAR)
	{
	  z.im = z.re;
	  z.re = xZero;
	  if ((endptr))
	    *endptr = ptr + 1;
	}
      else
	{
	  while (isspace (*ptr))
	    ptr++;
	  if (*ptr == '+' || *ptr == '-')
	    {
	      z.im = strtox (ptr, &ptr2);
	      if (*ptr2 != CX1I_CHAR)
		z.im = xZero;
	      else
		{
		  if ((endptr))
		    *endptr = ptr2 + 1;
		}
	    }
	  /*
	     else
	     : we have successfully read a real number
	     but there is no another number after it.
	     : So, we leave z.im set to zero and return z.
	  */
	}
    }
  else
    /* 
       : q does not contain any valid number
       ==> z.re is xNaN. Then we set z.im to
       xNaN and return z.
       We remark that, if endptr is
       not NULL, then *endptr == q.
    */
    z.im = xNaN;
  return z;
}

cxpr
atocx (const char *s)
{
  return strtocx (s, NULL);
}

char *
cxpr_asprint (cxpr z, int sc_not, int sign, int lim)
{
  char *str1, *str2, *t;

  str1 = xpr_asprint (z.re, sc_not, sign, lim);
  str2 = xpr_asprint (z.im, sc_not, 1, lim);
  if (!str1 || !str2)
    {
      if ((str1))
	free ((void*)str1);
      if ((str2))
	free ((void*)str2);
      return NULL;
    }
  else
    {
      size_t n = strlen (str1) + strlen (str2) + 2;

      if (!(t = (char*)malloc (n * sizeof (char))))
	return NULL;
      else
	{
	  strcpy (t, str1);
	  strcat (t, str2);
	  for (n = 0; t[n] != '\0'; n++);
	  t[n] = CX1I_CHAR;
	  t[n + 1] = '\0';
	  return t;
	}
    }
}

char *
cxtoa (cxpr z, int lim)
{
  return cxpr_asprint (z, 1, 0, lim);
}

cxpr
dctocx (double re, double im)
{
  cxpr z;

  z.re = dbltox (re);
  z.im = dbltox (im);
  return z;
}

cxpr
fctocx (float re, float im)
{
  cxpr z;

  z.re = flttox (re);
  z.im = flttox (im);
  return z;
}

cxpr
ictocx (long re, long im)
{
  cxpr z;

  z.re = inttox (re);
  z.im = inttox (im);
  return z;
}

cxpr uctocx (unsigned long re, unsigned long im)
{
  cxpr z;

  z.re = uinttox (re);
  z.im = uinttox (im);
  return z;
}

void
cxtodc (const cxpr *z, double *re, double *im)
{
  *re = xtodbl (z->re);
  *im = xtodbl (z->im);
}

void
cxtofc (const cxpr *z, float *re, float *im)
{
  *re = xtoflt (z->re);
  *im = xtoflt (z->im);
}
