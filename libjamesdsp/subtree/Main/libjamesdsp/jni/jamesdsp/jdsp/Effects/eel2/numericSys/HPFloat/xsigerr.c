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
#include<stdlib.h>
#include "xpre.h"

#ifdef XERR_EXIT
#define XERR_WARN
#endif /* XERR_EXIT ==> XERR_WARN */

/* The source code in this file is compiled only when */
/* the macro XERR_IGN is not defined.                 */
#ifndef XERR_IGN

#ifdef XERR_WARN

static const char *errmsg[] = {
  "No error",
  "Division by zero",
  "Out of domain",
  "Bad exponent",
  "Floating point overflow",
  "Invalid error code"
};

#else

int xErrNo = 0;

#endif /* XERR_WARN */

/*
  Remarks:

  errcode must come from the evaluation of an error condition.
  errcode, which should describe the type of the error, 
  should always be one between XEDIV, XEDOM, XEBADEXP and XFPOFLOW.
*/

int
xsigerr (int errcond, int errcode, const char *where)
{
  if (!errcond)
    errcode = 0;
  if (errcode < 0 || errcode > XNERR)
    errcode = XEINV;
#ifdef XERR_WARN
  if ((errcode))
    {
      if ((where))
	fprintf (stderr, "*** %s: %s\n", where, errmsg[errcode]);
      else
	fprintf (stderr, "*** %s\n", errmsg[errcode]);
#ifdef XERR_EXIT
      exit (EXIT_FAILURE);
#else
      return errcode;
#endif
    }
  return 0;
#else /* DEFAULT */
  if ((errcode))
    xErrNo = errcode;
  return errcode;
#endif
}

#endif /* XERR_IGN */
