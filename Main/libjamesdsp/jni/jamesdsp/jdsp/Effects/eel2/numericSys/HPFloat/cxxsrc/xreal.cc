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
#include "xreal.h"

using namespace std;

#define BUFF_SIZE 5120 /* 5 Kb */

const xreal xZERO = xZero;
const xreal xONE = xOne;
const xreal xTWO = xTwo;
const xreal xTEN = xTen;
const xreal xINF = xPinf;
const xreal x_INF = xMinf;
const xreal xNAN  = xNaN;
const xreal xPI = xPi;
const xreal xPI2 = xPi2;
const xreal xPI4 = xPi4;
const xreal xEE = xEe;
const xreal xSQRT2 = xSqrt2;
const xreal xLN2 = xLn2;
const xreal xLN10 = xLn10;
const xreal xLOG2_E = xLog2_e;
const xreal xLOG2_10 = xLog2_10;
const xreal xLOG10_E = xLog10_e;
const xreal x_floatMin = HPA_MIN;
const xreal x_floatMax = HPA_MAX;
xoutflags xreal::ioflags = { -1, XOUT_FIXED, 0, 0, 6, ' ', -1, -1 };

ostream&
operator<< (ostream& os, const xreal& x)
{
  char buffer[BUFF_SIZE];

  xsout (buffer, BUFF_SIZE, x.ioflags, x.br);
  return os << buffer;
}

istream&
operator>> (istream& is, xreal& x)
{
  double f;
  istream& res = is >> f;

  x.br = dbltox (f);
  return res;
}

xreal
operator+ (const xreal& x1, const xreal& x2)
{
  return xreal(xadd (x1.br, x2.br, 0));
}

xreal
operator- (const xreal& x1, const xreal& x2)
{
  return xreal(xadd (x1.br, x2.br, 1));
}

xreal
operator* (const xreal& x1, const xreal& x2)
{
  return xreal(xmul (x1.br, x2.br));
}

xreal
operator/ (const xreal& x1, const xreal& x2)
{
  return xreal(xdiv (x1.br, x2.br));
}

xreal
operator% (const xreal& x1, int n)
{
  return xpr2 (x1.br, n);
}

int
operator== (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) == 0);
}

int
operator!= (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) != 0);
}

int
operator<= (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) <= 0);
}

int
operator>= (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) >= 0);
}

int
operator< (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) < 0);
}

int
operator> (const xreal& x1, const xreal& x2)
{
  return (xprcmp (&x1.br, &x2.br) > 0);
}

unsigned long
sget (string s, unsigned long startp, xreal& x)
{
  const char *startptr;
  char *tail;

  if (startp >= s.length())
    {
      x.br = xZero;
      return 0;
    }
  else
    {
      startptr = s.c_str() + startp;
      x.br = strtox (startptr, &tail);
      return tail - startptr;
    }
}

const char*
bget (const char* buff, xreal& x)
{
  char* tail;

  if (!buff)
    {
      cerr << "*** HPA::xreal::bget(): the first argument is the null pointer" << endl;
      return 0;
    }
  else
    {
      x.br = strtox (buff, &tail);
      return (const char*) tail;
    }
}

int
compare (const xreal& x1, const xreal& x2)
{
  return xprcmp (&x1.br, &x2.br);
}

int
isNaN (const xreal& x)
{
  return xisNaN (&x.br);
}

xreal
abs (const xreal& s)
{
  return xabs (s.br);
}

xreal
frexp (const xreal& s, int *p)
{
  return xfrexp (s.br, p);
}

xreal
qfmod (const xreal& s, const xreal& t, xreal& q)
{
  return xfmod (s.br, t.br, &q.br);
}

xreal
fmod (const xreal& s, const xreal& t)
{
  xpr q;

  return xfmod (s.br, t.br, &q);
}

xreal
sfmod (const xreal& s, int *p)
{
  return xsfmod (s.br, p);
}

xreal
frac (const xreal& x)
{
  return xfrac (x.br);
}

xreal
trunc (const xreal& x)
{
  return xtrunc (x.br);
}

xreal
round (const xreal& x)
{
  return xround (x.br);
}

xreal
ceil (const xreal& x)
{
  return xceil (x.br);
}

xreal
floor (const xreal& x)
{
  return xfloor (x.br);
}

xreal
fix (const xreal& x)
{
  return xfix (x.br);
}

xreal
tan (const xreal& x)
{
  return xtan (x.br);
}

xreal
sin (const xreal& x)
{
  return xsin (x.br);
}

xreal
cos (const xreal& x)
{
  return xcos (x.br);
}

xreal
atan (const xreal& a)
{
  return xatan (a.br);
}

xreal
asin (const xreal& a)
{
  return xasin (a.br);
}

xreal
acos (const xreal& a)
{
  return xacos (a.br);
}

xreal
atan2 (const xreal& y, const xreal& x)
{
  return xatan2 (y.br, x.br);
}

xreal
sqrt (const xreal& u)
{
  return xsqrt (u.br);
}

xreal
exp (const xreal& u)
{
  return xexp (u.br);
}

xreal
exp2 (const xreal& u)
{
  return xexp2 (u.br);
}

xreal
exp10 (const xreal& u)
{
  return xexp10 (u.br);
}

xreal
log (const xreal& u)
{
  return xlog (u.br);
}

xreal
log2 (const xreal& u)
{
  return xlog2 (u.br);
}

xreal
log10 (const xreal& u)
{
  return xlog10 (u.br);
}

xreal
tanh (const xreal& v)
{
  return xtanh (v.br);
}

xreal
sinh (const xreal& v)
{
  return xsinh (v.br);
}

xreal
cosh (const xreal& v)
{
  return xcosh (v.br);
}

xreal
atanh (const xreal& v)
{
  return xatanh (v.br);
}

xreal
asinh (const xreal& v)
{
  return xasinh (v.br);
}

xreal
acosh (const xreal& v)
{
  return xacosh (v.br);
}

xreal
pow (const xreal& x, const xreal& y)
{
  return xpow (x.br, y.br);
}

extern int
hpa_read_item (istream& is, char* buff, unsigned size);

int
xreal::getfrom (istream& is)
{
  char buffer[BUFF_SIZE];
  int n = hpa_read_item (is, buffer, BUFF_SIZE);

  br = atox (buffer);
  return n;
}

int 
xreal::print (ostream& os, int sc_not, int sign, int lim) const
{
  char* s = xpr_asprint (br, sc_not, sign, lim);

  if (!s)
    return -1;
  else
    {
      os << s;
      free ((void*) s);
      return 0;
    }
}

extern "C" {

extern int xErrNo;

}

int
xmatherrcode ()
{
#ifdef XERR_DFL
  return xErrNo;
#else
  return -1;
#endif
}

void
clear_xmatherr ()
{
#ifdef XERR_DFL
  xErrNo = 0;
#else
  cerr << "*** HPA: Feature not available, sorry" << endl;
#endif
}
