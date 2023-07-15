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

#include <cstdlib>
#include "xcomplex.h"

using namespace std;

#define BUFF_SIZE 10240 /* 10 Kb */

  const xcomplex cxZERO = cxZero;
  const xcomplex cxONE = cxOne;
  const xcomplex cxI = cxIU;

xoutflags xcomplex::ioflags = { XFMT_STD, XOUT_FIXED, 0, 0, 6, ' ', CXDEF_LDEL, CXDEF_RDEL };

ostream&
operator<< (ostream& os, const xcomplex& x)
{
  char buffer[BUFF_SIZE];

  cxsout (buffer, BUFF_SIZE, x.ioflags, x.br);
  return os << buffer;
}

istream&
operator>> (istream& is, xcomplex& x)
{
  double_complex z;
  istream& res = is >> z.re >> z.im;

  x.br = dctocx (z.re, z.im);
  return res;
}

xcomplex
operator+ (const xcomplex& x1, const xcomplex& x2)
{
  return xcomplex(cxadd (x1.br, x2.br, 0));
}

xcomplex
operator- (const xcomplex& x1, const xcomplex& x2)
{
  return xcomplex(cxadd (x1.br, x2.br, 1));
}

xcomplex
operator* (const xcomplex& x1, const xcomplex& x2)
{
  return xcomplex(cxmul (x1.br, x2.br));
}

xcomplex
operator/ (const xcomplex& x1, const xcomplex& x2)
{
  return xcomplex(cxdiv (x1.br, x2.br));
}

xcomplex
operator% (const xcomplex& x1, int n)
{
  cxpr z;

  z.re = xpr2 (x1.br.re, n);
  z.im = xpr2 (x1.br.im, n);
  return xcomplex (z);
}

int
operator== (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  return (t.re == 0 && t.im == 0);
}

int
operator!= (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  return (t.re != 0 || t.im != 0);
}

int
operator<= (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  return (t.re <= 0 && t.im <= 0);
}

int
operator>= (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  return (t.re >= 0 && t.im >= 0);
}

int
operator< (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  if (t.re < 0)
    return (t.im <= 0);
  else if (t.im < 0)
    return (t.re <= 0);
  else
    return 0;
}

int
operator> (const xcomplex& x1, const xcomplex& x2)
{
  cxprcmp_res t;

  t.re = xprcmp (&x1.br.re, &x2.br.re);
  t.im = xprcmp (&x1.br.im, &x2.br.im);
  if (t.re > 0)
    return (t.im >= 0);
  else if (t.im > 0)
    return (t.re >= 0);
  else
    return 0;
}

unsigned long
sget (string s, unsigned long startp, xcomplex& x)
{
  const char *startptr;
  char *tail;

  if (startp >= s.length())
    {
      x.br = cxZero;
      return 0;
    }
  else
    {
      startptr = s.c_str() + startp;
      x.br = strtocx (startptr, &tail);
      return tail - startptr;
    }
}

const char*
bget (const char* buff, xcomplex& x)
{
  char* tail;

  if (!buff)
    {
      cerr << "*** HPA::xcomplex::bget(): the first argument is the null pointer" << endl;
      return 0;
    }
  else
    {
      x.br = strtocx (buff, &tail);
      return (const char*) tail;
    }
}

xcomplex 
rmul (const xreal& x, const xcomplex& z)
{
  return xcomplex (cxrmul (x._2xpr(), z.br));
}

xcomplex
gdiv (const xcomplex& x1, const xcomplex& x2)
{
  return cxgdiv (x1.br, x2.br);
}

xcomplex
gmod (const xcomplex& x1, const xcomplex& x2)
{
  return cxgmod (x1.br, x2.br);
}

xcomplex
idiv (const xcomplex& x1, const xcomplex& x2)
{
  return cxidiv (x1.br, x2.br);
}

xcomplex
mod (const xcomplex& x1, const xcomplex& x2)
{
  return cxmod (x1.br, x2.br);
}

xcomplex
conj (const xcomplex& z)
{
  return cxconj (z.br);
}

xcomplex
inv  (const xcomplex& z)
{
  return cxinv (z.br);
}

xcomplex
swap (const xcomplex& z)
{
  return cxswap (z.br);
}

xcomplex
drot (const xcomplex& z)
{
  return cxdrot (z.br);
}

xcomplex
rrot (const xcomplex& z)
{
  return cxrrot (z.br);
}

xreal
abs (const xcomplex& s)
{
  return xreal (cxabs (s.br));
}

xreal
arg (const xcomplex& s)
{
  return xreal (cxarg (s.br));
}

/*
  Some stuff must be added here
*/

xcomplex
frac (const xcomplex& x)
{
  return cxfrac (x.br);
}

xcomplex
trunc (const xcomplex& x)
{
  return cxtrunc (x.br);
}

xcomplex
round (const xcomplex& x)
{
  return cxround (x.br);
}

xcomplex
ceil (const xcomplex& x)
{
  return cxceil (x.br);
}

xcomplex
floor (const xcomplex& x)
{
  return cxfloor (x.br);
}

xcomplex
fix (const xcomplex& x)
{
  return cxfix (x.br);
}

xcomplex
tan (const xcomplex& x)
{
  return cxtan (x.br);
}

xcomplex
sin (const xcomplex& x)
{
  return cxsin (x.br);
}

xcomplex
cos (const xcomplex& x)
{
  return cxcos (x.br);
}

xcomplex
atan (const xcomplex& a)
{
  return cxatan (a.br);
}

xcomplex
asin (const xcomplex& a)
{
  return cxasin (a.br);
}

xcomplex
acos (const xcomplex& a)
{
  return cxacos (a.br);
}

xcomplex
sqr (const xcomplex& u)
{
  return cxsqr (u.br);
}

xcomplex
sqrt (const xcomplex& u)
{
  return cxsqrt (u.br);
}

xcomplex
root (const xcomplex& u, int i, int n)
{
  return cxroot (u.br, i, n);
}

xcomplex
exp (const xcomplex& u)
{
  return cxexp (u.br);
}

xcomplex
exp2 (const xcomplex& u)
{
  return cxexp2 (u.br);
}

xcomplex
exp10 (const xcomplex& u)
{
  return cxexp10 (u.br);
}

xcomplex
log (const xcomplex& u)
{
  return cxlog (u.br);
}

xcomplex
log2 (const xcomplex& u)
{
  return cxlog2 (u.br);
}

xcomplex
log10 (const xcomplex& u)
{
  return cxlog10 (u.br);
}

xcomplex
log_sqrt (const xcomplex& u)
{
  return cxlog_sqrt (u.br);
}

xcomplex
tanh (const xcomplex& v)
{
  return cxtanh (v.br);
}

xcomplex
sinh (const xcomplex& v)
{
  return cxsinh (v.br);
}

xcomplex
cosh (const xcomplex& v)
{
  return cxcosh (v.br);
}

xcomplex
atanh (const xcomplex& v)
{
  return cxatanh (v.br);
}

xcomplex
asinh (const xcomplex& v)
{
  return cxasinh (v.br);
}

xcomplex
acosh (const xcomplex& v)
{
  return cxacosh (v.br);
}

xcomplex
pow (const xcomplex& x, const xcomplex& y)
{
  return cxpow (x.br, y.br);
}

extern int
hpa_read_item (istream& is, char* buff, unsigned size);

int
xcomplex::getfrom (istream& is)
{
  char buffer[BUFF_SIZE];
  int n = hpa_read_item (is, buffer, BUFF_SIZE);

  br = atocx (buffer);
  return n;
}

int 
xcomplex::print (ostream& os, int sc_not, int sign, int lim) const
{
  char* s = cxpr_asprint (br, sc_not, sign, lim);

  if (!s)
    return -1;
  else
    {
      os << s;
      free ((void*) s);
      return 0;
    }
}
