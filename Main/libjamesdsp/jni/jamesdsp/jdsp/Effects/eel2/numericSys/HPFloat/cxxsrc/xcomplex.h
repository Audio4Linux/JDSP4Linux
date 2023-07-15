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

#ifndef _XCOMPLEX_H_
#define _XCOMPLEX_H_

#include "../cxpre.h"
#include <iostream>
#include <string>
#include "xreal.h"

using std::istream;
using std::ostream;
using std::string;

  struct double_complex {
    double re, im;
  };

  struct float_complex {
    float re, im;
  };

typedef struct xcomplex
{
  friend ostream& operator<< (ostream& os, const xcomplex& x);
  friend istream& operator>> (istream& is, xcomplex& x);
  friend xcomplex operator+ (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex operator- (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex operator* (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex operator/ (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex operator% (const xcomplex& x1, int n);
  friend int operator== (const xcomplex& x1, const xcomplex& x2);
  friend int operator!= (const xcomplex& x1, const xcomplex& x2);
  friend int operator<= (const xcomplex& x1, const xcomplex& x2);
  friend int operator>= (const xcomplex& x1, const xcomplex& x2);
  friend int operator< (const xcomplex& x1, const xcomplex& x2);
  friend int operator> (const xcomplex& x1, const xcomplex& x2);
  /* Friend functions */
  friend unsigned long sget (string s, unsigned long startptr, xcomplex& x);
  friend const char* bget (const char* buff, xcomplex& x);
  friend xcomplex rmul (const xreal& x, const xcomplex& z);
  friend xcomplex gdiv (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex gmod (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex idiv (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex mod (const xcomplex& x1, const xcomplex& x2);
  friend xcomplex conj (const xcomplex& z);
  friend xcomplex inv  (const xcomplex& z);
  friend xcomplex swap (const xcomplex& z);
  friend xcomplex drot (const xcomplex& z);
  friend xcomplex rrot (const xcomplex& z);
  friend xreal abs (const xcomplex& s);
  friend xreal arg (const xcomplex& s);
  friend xcomplex frac (const xcomplex& x);
  friend xcomplex trunc (const xcomplex& x);
  friend xcomplex round (const xcomplex& x);
  friend xcomplex ceil (const xcomplex& x);
  friend xcomplex floor (const xcomplex& x);
  friend xcomplex fix (const xcomplex& x);
  friend xcomplex tan (const xcomplex& x);
  friend xcomplex sin (const xcomplex& x);
  friend xcomplex cos (const xcomplex& x);
  friend xcomplex atan (const xcomplex& a);
  friend xcomplex asin (const xcomplex& a);
  friend xcomplex acos (const xcomplex& a);
  friend xcomplex sqr (const xcomplex& u);
  friend xcomplex sqrt (const xcomplex& u);
  friend xcomplex root (const xcomplex& u, int i, int n);
  friend xcomplex exp (const xcomplex& u);
  friend xcomplex exp2 (const xcomplex& u);
  friend xcomplex exp10 (const xcomplex& u);
  friend xcomplex log (const xcomplex& u);
  friend xcomplex log2 (const xcomplex& u);
  friend xcomplex log10 (const xcomplex& u);
  friend xcomplex log_sqrt (const xcomplex& u);
  friend xcomplex tanh (const xcomplex& v);
  friend xcomplex sinh (const xcomplex& v);
  friend xcomplex cosh (const xcomplex& v);
  friend xcomplex atanh (const xcomplex& v);
  friend xcomplex asinh (const xcomplex& v);
  friend xcomplex acosh (const xcomplex& v);
  friend xcomplex pow (const xcomplex& x, const xcomplex& y);
  
  xcomplex (const cxpr* px = &cxZero) : br(*px) { }
  xcomplex (cxpr x) : br(x) { }
  xcomplex (xpr x, xpr y = xZero) {
    br.re = x, br.im = y;
  }
  xcomplex (xreal x, xreal y = xZERO) {
    br.re = x._2xpr(), br.im = y._2xpr();
  }
  xcomplex (double x, double y = 0.0) {
    br = dctocx (x, y);
  }
  xcomplex (float x, float y = 0.0) {
    br = fctocx (x, y);
  }
  xcomplex (int m, int n = 0) {
    br = ictocx (m, n);
  }
  xcomplex (long m, long n = 0) {
    br = ictocx (m, n);
  }
  xcomplex (unsigned int u, unsigned int v = 0U) {
    br = uctocx (u, v);
  }
  xcomplex (unsigned long u, unsigned long v = 0U) {
    br = uctocx (u, v);
  }
  xcomplex (const char* str, char** endptr = 0) {
    br = strtocx (str, endptr);
  }
  xcomplex (string str) {
    br = atocx (str.c_str ());
  }
  xcomplex (const xcomplex& x) {
    br = x.br;
  }
  /* Assignment operators */
  xcomplex& operator= (const xcomplex& x) {
    br = x.br; 
    return *this;
  }
  xcomplex& operator+= (const xcomplex& x) {
    br = cxadd (br, x.br, 0); 
    return *this;
  }
  xcomplex& operator-= (const xcomplex& x) {
    br = cxadd (br, x.br, 1); 
    return *this;
  }
  xcomplex& operator*= (const xcomplex& x) {
    br = cxmul (br, x.br);
    return *this;
  }
  xcomplex& operator*= (const xreal& x) {
    br = cxrmul (x._2xpr(), br);
    return *this;
  }
  xcomplex& operator/= (const xcomplex& x) {
    br = cxdiv (br, x.br);
    return *this;
  }
  xcomplex& operator%= (int n) {
    br.re = xpr2 (br.re, n);
    br.im = xpr2 (br.im, n);
    return *this;
  }
  /* Increment operators */
  xcomplex& operator++ () {
    br = cxadd (br, cxOne, 0); 
    return *this;
  }
  xcomplex& operator-- () {
    br = cxadd (br, cxOne, 1); 
    return *this;
  }
  xcomplex& operator++ (int dummy) {
    br = cxadd (br, cxOne, 0); 
    return *this;
  }
  xcomplex& operator-- (int dummy) {
    br = cxadd (br, cxOne, 1); 
    return *this;
  }
  /* Destructor */
  ~xcomplex (void) {
    br = cxZero;
  }
  /* Integer exponent power */
  xcomplex operator() (int n) const {
    return cxpwr (br, n);
  }
  /* Negation */
  xcomplex operator-() const {
    return cxneg (br);
  }
  int operator!() const {
    return cxis0 (&br);
  }
  /* Functions for conversions */
  double_complex _2dcomplex () const {
    double_complex z;
    
    cxtodc (&br, &z.re, &z.im);
    return z;
  }
  float_complex _2fcomplex() const {
    float_complex z;
    
    cxtofc (&br, &z.re, &z.im);
    return z;
  }
  cxpr _2cxpr() const {
    return br;
  }
  string _2string() const {
    char* s = cxpr_asprint (br, 1, 1, (XDIM * 48) / 10 - 2);
    string str(s);

    free ((void*) s);
    return str;
  }
  /* Real and imaginary part */
  xreal real () const {
    return xreal (br.re);
  }
  xreal imag () const {
    return xreal (br.im);
  }
  xpr _real () const {
    return br.re;
  }
  xpr _imag () const {
    return br.im;
  }
  double dreal () const {
    return xtodbl (br.re);
  }
  double dimag () const {
    return xtodbl (br.im);
  }
  double freal () const {
    return xtoflt (br.re);
  }
  double fimag () const {
    return xtoflt (br.im);
  }  
  string sreal () const {
    char* s = xpr_asprint (br.re, 1, 1, (XDIM * 48) / 10 - 2);
    string str(s);

    free ((void*) s);
    return str;
  }
  string simag () const {
    char* s = xpr_asprint (br.im, 1, 1, (XDIM * 48) / 10 - 2);
    string str(s);

    free ((void*) s);
    return str;
  }
  void real (const xreal& x) {
    br.re = x._2xpr();
  }
  void imag (const xreal& x) {
    br.im = x._2xpr();
  }
  void real (xpr x) {
    br.re = x;
  }
  void imag (xpr x) {
    br.im = x;
  }
  void real (const xpr* px) {
    br.re = *px;
  }
  void imag (const xpr* px) {
    br.im = *px;
  }
  void real (double x) {
    br.re = dbltox(x);
  }
  void imag (double x) {
    br.im = dbltox(x);
  }
  void real (float x) {
    br.re = flttox(x);
  }
  void imag (float x) {
    br.im = flttox(x);
  }
  void real (int x) {
    br.re = inttox(x);
  }
  void imag (int x) {
    br.im = inttox(x);
  }
  void real (long x) {
    br.re = inttox(x);
  }
  void imag (long x) {
    br.im = inttox(x);
  }
  void real (unsigned int x) {
    br.re = uinttox(x);
  }
  void imag (unsigned int x) {
    br.im = uinttox(x);
  }
  void real (unsigned long x) {
    br.re = uinttox(x);
  }
  void imag (unsigned long x) {
    br.im = uinttox(x);
  }
  void real (const char* str, char** endptr = 0) {
    br.re = strtox (str, endptr);
  }
  void imag (const char* str, char** endptr = 0) {
    br.im = strtox (str, endptr);
  }
  void real (string str) {
    br.re = atox (str.c_str ());
  }
  void imag (string str) {
    br.im = atox (str.c_str ());
  }
  int getfrom (istream& is);
  int print (ostream& os, int sc_not, int sign, int lim) const;
  char* asprint (int sc_not, int sign, int lim) const {
    return cxpr_asprint (br, sc_not, sign, lim);
  }
  static void set_fmt (short format) {
    ioflags.fmt = format;
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
  static void set_ldelim (signed char ch) {
    ioflags.ldel = ch;
  }
  static void set_rdelim (signed char ch) {
    ioflags.rdel = ch;
  }
  static short get_fmt (void) {
    return ioflags.fmt;
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
  static signed char get_ldelim (void) {
    return ioflags.ldel;
  }
  static signed char get_rdelim (void) {
    return ioflags.rdel;
  }
  cxpr br; /* binary representation */
  static xoutflags ioflags; /* output flags */
} ComplexNumber;

 extern const xcomplex cxZERO, cxONE, cxI;
#define xi cxI
#define xj cxI
#define _i cxI
#define _j cxI

#endif /* _XCOMPLEX_H_ */
