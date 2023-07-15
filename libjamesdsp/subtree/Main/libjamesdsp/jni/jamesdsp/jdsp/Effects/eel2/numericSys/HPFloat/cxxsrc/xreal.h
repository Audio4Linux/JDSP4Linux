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

#ifndef _XREAL_H_
#define _XREAL_H_

#include "../xpre.h"
#include <iostream>
#include <string>
#include <cstdlib>

using std::ostream;
using std::istream;
using std::string;

typedef struct xreal
{
  friend ostream& operator<< (ostream& os, const xreal& x);
  friend istream& operator>> (istream& is, xreal& x);
  friend xreal operator+ (const xreal& x1, const xreal& x2);
  friend xreal operator- (const xreal& x1, const xreal& x2);
  friend xreal operator* (const xreal& x1, const xreal& x2);
  friend xreal operator/ (const xreal& x1, const xreal& x2);
  friend xreal operator% (const xreal& x1, int n);
  friend int operator== (const xreal& x1, const xreal& x2);
  friend int operator!= (const xreal& x1, const xreal& x2);
  friend int operator<= (const xreal& x1, const xreal& x2);
  friend int operator>= (const xreal& x1, const xreal& x2);
  friend int operator< (const xreal& x1, const xreal& x2);
  friend int operator> (const xreal& x1, const xreal& x2);
  /* Friend functions */
  friend unsigned long sget (string s, unsigned long startptr, xreal& x);
  friend const char* bget (const char* buff, xreal& x);
  friend int compare (const xreal& x1, const xreal& x2);
  friend int isNaN (const xreal& x);
  friend xreal abs (const xreal& s);
  friend xreal frexp (const xreal& s, int *p);
  friend xreal qfmod (const xreal& s, const xreal& t, xreal& q);
  friend xreal fmod (const xreal& s, const xreal& t);
  friend xreal sfmod (const xreal& s, int *p);
  friend xreal frac (const xreal& x);
  friend xreal trunc (const xreal& x);
  friend xreal round (const xreal& x);
  friend xreal ceil (const xreal& x);
  friend xreal floor (const xreal& x);
  friend xreal fix (const xreal& x);
  friend xreal tan (const xreal& x);
  friend xreal sin (const xreal& x);
  friend xreal cos (const xreal& x);
  friend xreal atan (const xreal& a);
  friend xreal atan2 (const xreal& y, const xreal& x);
  friend xreal asin (const xreal& a);
  friend xreal acos (const xreal& a);
  friend xreal sqrt (const xreal& u);
  friend xreal exp (const xreal& u);
  friend xreal exp2 (const xreal& u);
  friend xreal exp10 (const xreal& u);
  friend xreal log (const xreal& u);
  friend xreal log2 (const xreal& u);
  friend xreal log10 (const xreal& u);
  friend xreal tanh (const xreal& v);
  friend xreal sinh (const xreal& v);
  friend xreal cosh (const xreal& v);
  friend xreal atanh (const xreal& v);
  friend xreal asinh (const xreal& v);
  friend xreal acosh (const xreal& v);
  friend xreal pow (const xreal& x, const xreal& y);
  
  xreal (const xpr* px = &xZero) : br(*px) { }
  xreal (xpr x) : br(x) { }
  xreal (double x) {
    br = dbltox (x);
  }
  xreal (float x) {
    br = flttox (x);
  }
  xreal (int n) {
    br = inttox (n);
  }
  xreal (long n) {
    br = inttox (n);
  }
  xreal (unsigned int u) {
    br = uinttox (u);
  }
  xreal (unsigned long u) {
    br = uinttox (u);
  }
  xreal (const char* str, char** endptr = 0) {
    br = strtox (str, endptr);
  }
  xreal (string str) {
    br = atox (str.c_str ());
  }
  xreal (const xreal& x) {
    br = x.br;
  }
  /* Assignment operators */
  xreal& operator= (const xreal& x) {
    br = x.br; 
    return *this;
  }
  xreal& operator+= (const xreal& x) {
    br = xadd (br, x.br, 0); 
    return *this;
  }
  xreal& operator-= (const xreal& x) {
    br = xadd (br, x.br, 1); 
    return *this;
  }
  xreal& operator*= (const xreal& x) {
    br = xmul (br, x.br);
    return *this;
  }
  xreal& operator/= (const xreal& x) {
    br = xdiv (br, x.br);
    return *this;
  }
  xreal& operator%= (int n) {
    br = xpr2 (br, n);
    return *this;
  }
  /* Increment operators */
  xreal& operator++ () {
    br = xadd (br, xOne, 0); 
    return *this;
  }
  xreal& operator-- () {
    br = xadd (br, xOne, 1); 
    return *this;
  }
  xreal& operator++ (int dummy) {
    br = xadd (br, xOne, 0); 
    return *this;
  }
  xreal& operator-- (int dummy) {
    br = xadd (br, xOne, 1); 
    return *this;
  }
  /* Destructor */
  ~xreal (void) {
    br = xZero;
  }
  /* Integer exponent power */
  xreal operator() (int n) const {
    return xpwr (br, n);
  }
  /* Negation */
  xreal operator-() const {
    return xneg (br);
  }
  int operator!() const {
    return xis0 (&br);
  }
  int isneg() const {
    return x_neg ((xpr*)&br);
  }
  int exp() const {
    return x_exp ((xpr*)&br);
  }
  /* Functions for conversions */
  double _2double () const {
    return xtodbl(br);
  }
  float _2float() const {
    return xtoflt(br);
  }
  xpr _2xpr() const {
    return br;
  }
  string _2string() const {
    char* s = xpr_asprint (br, 1, 1, (XDIM * 48) / 10 - 2);
    string str(s);

    free ((void*) s);
    return str;
  }
  int getfrom (istream& is);
  int print (ostream& os, int sc_not, int sign, int lim) const;
  char* asprint (int sc_not, int sign, int lim) const {
    return xpr_asprint (br, sc_not, sign, lim);
  }
  static void set_notation (short notat) {
    ioflags.notat = notat;
  }
  static void set_signflag (short onoff) {
    ioflags.sf = onoff;
  }
  static void set_mfwd (short wd) {
    ioflags.mfwd = wd;
  }
  static void set_lim (short lim) {
    ioflags.lim = lim;
  }
  static void set_padding (signed char ch) {
    ioflags.padding = ch;
  }
  static short get_notation (void) {
    return ioflags.notat;
  }
  static short get_signflag (void) {
    return ioflags.sf;
  }
  static short get_mfwd (void) {
    return ioflags.mfwd;
  }
  static short get_lim (void) {
    return ioflags.lim;
  }
  static signed char get_padding (void) {
    return ioflags.padding;
  }
  xpr br; /* binary representation */
  static xoutflags ioflags; /* output flags */
} RealNumber;

 int xmatherrcode ();
 void clear_xmatherr ();

 extern const xreal xZERO, xONE, xTWO, xTEN;
 extern const xreal xINF, x_INF, xNAN;
 extern const xreal xPI, xPI2, xPI4, xEE, xSQRT2;
 extern const xreal xLN2, xLN10, xLOG2_E, xLOG2_10, xLOG10_E, x_floatMin, x_floatMax;

#endif /* _XREAL_H_ */
