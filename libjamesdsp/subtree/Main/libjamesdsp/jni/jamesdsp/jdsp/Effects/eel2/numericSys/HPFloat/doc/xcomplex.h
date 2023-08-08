#ifndef _XCOMPLEX_H_
#define _XCOMPLEX_H_

#include <cxpre.h>
#include <iostream>
#include <string>
#include "xreal.h"

using std::istream;
using std::ostream;
using std::string;

namespace HPA {

  struct double_complex {
    double re, im;
  };

  struct float_complex {
    float re, im;
  };

class xcomplex {
  // << and >> are used respectively for the output and the
  // input of extended precision complex numbers.
  // The input operator >> reads a couple of double
  // precision numbers and then converts it into
  // an extended precision complex number. This can have 
  // undesirable rounding effects. To avoid them, use the
  // input function xcomplex::getfrom() (see below).
  friend ostream& operator<< (ostream& os, const xcomplex& z);
  friend istream& operator>> (istream& is, xcomplex& z);

  // +, -, *, / are the usual arithmetic operators
  friend xcomplex
    operator+ (const xcomplex& z1, const xcomplex& z2);
  friend xcomplex
    operator- (const xcomplex& z1, const xcomplex& z2);
  friend xcomplex 
    operator* (const xcomplex& z1, const xcomplex& z2);
  friend xcomplex
    operator/ (const xcomplex& z1, const xcomplex& z2);

  // z % n is equal to z * pow (2,n)
  friend xcomplex
    operator% (const xcomplex& z, int n);

  // ==, !=, <=, >=, <, > are the usual comparison operators
  friend int
    operator== (const xcomplex& z1, const xcomplex& z2);
  friend int
    operator!= (const xcomplex& z1, const xcomplex& z2);
  friend int
    operator<= (const xcomplex& z1, const xcomplex& z2);
  friend int
    operator>= (const xcomplex& z1, const xcomplex& z2);
  friend int
    operator< (const xcomplex& z1, const xcomplex& z2);
  friend int
    operator> (const xcomplex& z1, const xcomplex& z2);

  // sget (s, n, z) tries to read an extended precision
  // complex number from the string 's' starting from the
  // position 'n'. The retrieved number is converted and
  // stored in 'z'. The return value is the number of
  // characters composing the decimal representation of this
  // number as read from 's'. 
  // For example, if s == "12.34+6.7idog" and n == 0,
  // then  'z'  is set to  12.34+6.7i  and the return value
  // is 10.
  // If the portion of 's' starting from the position 'n' can
  // not be converted to a number, then 'z' is set to 
  // xNAN + xNANi and 0 is returned.
  // If the exactly converted value would cause overflow in
  // the real or/and imaginary part, then the real or/and the
  // imaginary part of 'z' are set to xINF or x_INF, according
  // to the signs of the correct value.
  // If 'n' is greater or equal to the length of 's', then 0
  // is returned and 'z' is set to cxZERO.
  friend unsigned long sget (string s, unsigned long startptr,
                             xcomplex& z);

  // bget (buff, z) tries to read an extended precision
  // complex number from the buffer pointed to by 'buff'. 
  // The retrieved number is converted and stored in 'z'. 
  // The return value is a pointer to the character after
  // the last character used in the conversion.
  // For example, if 'buff' is a pointer to the buffer 
  // "12.34+6.7idog", then 'z' is set to  12.34+6.7i  and 
  // the return value is a pointer to "dog" (i.e.,
  // a pointer to the character 'd').
  // If the initial portion of the string pointed to by 'buff'
  // can not be converted to a number, then 'z' is set to 
  // xNAN + xNANi and 'buff' is returned.
  // If the exactly converted value would cause overflow
  // in the real or/and imaginary part, then the real or/and
  // the imaginary part of 'z' are set to xINF or x_INF, 
  // according to the signs of the correct value.
  // If 'buff' is NULL (0), then an error message is printed
  // on 'cerr' (standard error device).
  friend const char* bget (const char* buff, xcomplex& z);

  // rmul (x,z) (here 'x' is a real number) returns the
  // product x * z.
  // It is faster than the * operator.
  friend xcomplex rmul (const xreal& x, const xcomplex& z);

  // After eventually rounding 'z1' and 'z2' by recalling
  // round() (see below) on them, gdiv(z1, z2) returns
  // the quotient of the gaussian division of 'z1' by 'z2'.
  // If you do not know what gaussian division means, probably
  // you will never need this function :)
  friend xcomplex
    gdiv (const xcomplex& z1, const xcomplex& z2);
  
  // After eventually rounding 'z1' and 'z2' by recalling
  // round() (see below) on them, gmod(z1, z2) returns
  // the remainder of the gaussian division of 'z1' by 'z2'.
  // If you do not know what gaussian division means, probably
  // you will never need this function :)
  friend xcomplex 
    gmod (const xcomplex& z1, const xcomplex& z2);

  // idiv() is a wrapper to cxidiv() (see section 
  // "Extended Precision Complex Arithmetic").
  friend xcomplex 
    idiv (const xcomplex& z1, const xcomplex& z2);

  // mod() is a wrapper to cxmod() (see section 
  // "Extended Precision Complex Arithmetic").
  friend xcomplex 
    mod (const xcomplex& z1, const xcomplex& z2);

  // conj() returns the complex conjugate of its argument.
  friend xcomplex conj (const xcomplex& z);

  // inv() returns the complex reciprocal of its argument:
  // inv(z) == 1/z .
  friend xcomplex inv  (const xcomplex& z);

  // swap(z) returns the complex number {z.im, z.re}.
  friend xcomplex swap (const xcomplex& z);

  // Multiplication by 1i (imaginary unit). 
  friend xcomplex drot (const xcomplex& z);

  // Multiplication by -1i
  friend xcomplex rrot (const xcomplex& z);

  // abs() returns the absolute value (or modulus) of its
  // argument.
  // The return value of abs() is then an 'xreal' number.
  friend xreal abs (const xcomplex& z);

  // arg(z) returns the phase angle (or argument)
  // of the complex number 'z'.
  // The return value of arg() is an 'xreal' number
  // in the range [-xPI, xPI) (-xPI is included, xPI is excluded).
  // If 'z' is null, then a domain-error is produced.
  friend xreal arg (const xcomplex& z);

  // The next six functions have the same
  // meanings of the corresponding real functions,
  // but they affect both the real
  // and the imaginary part of their argument.
  friend xcomplex frac (const xcomplex& z);
  friend xcomplex trunc (const xcomplex& z);
  friend xcomplex round (const xcomplex& z);
  friend xcomplex ceil (const xcomplex& z);
  friend xcomplex floor (const xcomplex& z);
  friend xcomplex fix (const xcomplex& z);

  // sqr() returns the square of its argument.
  friend xcomplex sqr (const xcomplex& z);

  // sqrt() returns the principal branch of the square root
  // of its argument.
  friend xcomplex sqrt (const xcomplex& z);

  // root (z,i,n) returns the 'i'th branch of the 'n'th root
  // of 'z'. If 'n' is zero or negative and 'z' is
  // zero, then a bad-exponent error is produced.
  friend xcomplex root (const xcomplex& z, int i, int n);

  // These functions do not require any comment, except that
  // tan() and tanh() yield a domain-error in the same cases
  // as cxtan() and cxtanh(), respectively.
  friend xcomplex exp (const xcomplex& z);
  friend xcomplex exp2 (const xcomplex& z);
  friend xcomplex exp10 (const xcomplex& z);
  friend xcomplex tan (const xcomplex& z);
  friend xcomplex sin (const xcomplex& z);
  friend xcomplex cos (const xcomplex& z);
  friend xcomplex tanh (const xcomplex& z);
  friend xcomplex sinh (const xcomplex& z);
  friend xcomplex cosh (const xcomplex& z);

  // Natural, base-2 and base-10 logarithm of a complex 
  // number.
  // A null argument results in a domain-error.
  // The imaginary part of the return value of log() is always
  // in the interval [-xPI,xPI) (-xPI is included, xPI is excluded).
  friend xcomplex log (const xcomplex& z);
  friend xcomplex log2 (const xcomplex& z);
  friend xcomplex log10 (const xcomplex& z);
  
  // log_sqrt(z) returns the natural logarithm of the
  // principal branch of the square root of 'z'.
  // A null argument results in a domain-error.
  // The imaginary part of the return value of log_sqrt()
  // is always in the interval [-xPI2,xPI2) (-xPI2 is included, xPI2 is excluded).
  friend xcomplex log_sqrt (const xcomplex& z);
  
  // These functions are self-explanatory. atan(z)
  // yields a domain-error if the real part of 'z' is null and
  // the imaginary part is equal to '+1' or '-1'.
  // Similarly, atanh(z) yields a domain-error if the 
  // imaginary part of 'z' is null and the
  // real part is equal to '+1' or '-1'.
  friend xcomplex atan (const xcomplex& z);
  friend xcomplex asin (const xcomplex& z);
  friend xcomplex acos (const xcomplex& z);
  friend xcomplex atanh (const xcomplex& z);
  friend xcomplex asinh (const xcomplex& z);
  friend xcomplex acosh (const xcomplex& z);

  // The return value of pow() is the power of the first
  // argument raised to the second one.
  // Note that the modulus of the first argument must be
  // greater than zero, if the real part of
  // the second argument is less or equal than zero, otherwise
  // a bad-exponent error is produced.
  friend xcomplex pow (const xcomplex& z1, const xcomplex& z2);
  
 public:
  // Various constructors. They allow to define
  // an extended precision complex number in several ways.
  // In addition, they allow for conversions from other
  // numeric types.
  xcomplex (const struct cxpr* pz = &cxZero);
  xcomplex (struct cxpr z);
  xcomplex (struct xpr x, struct xpr y = xZero);
  xcomplex (xreal x, xreal y = xZERO);
  xcomplex (double x, double y = 0.0);
  xcomplex (float x, float y = 0.0);
  xcomplex (int m, int n = 0);
  xcomplex (long m, long n = 0);
  xcomplex (unsigned int u, unsigned int v = 0U);
  xcomplex (unsigned long u, unsigned long v = 0U);
  
  // This constructor requires a special comment. If
  // only the first argument is present, the initial portion
  // of the string pointed to by this argument is converted
  // into an extended precision complex number, if a 
  // conversion is possible. If no conversion is possible,
  // then the returned number is xNAN + xNANi. 
  // If the second argument is present and is not null,
  // it must be the address of a valid pointer to 'char'.
  // Before returning, the constructor will set this pointer
  // so that it points to the character after the last 
  // character used in the conversion.
  xcomplex (const char* str, char** endptr = 0);

  xcomplex (string str);
  xcomplex (const xcomplex& z);

  // Assignment operators. They do not require
  // any explanation with the only exception of '%=',
  // which combines a '%' operation with an assignment.
  // So, x %= n is equivalent to x *= pow(2,n) .
  xcomplex& operator= (const xcomplex& z);
  xcomplex& operator+= (const xcomplex& z);
  xcomplex& operator-= (const xcomplex& z);
  xcomplex& operator*= (const xcomplex& z);
  xcomplex& operator*= (const xreal& x);
  xcomplex& operator/= (const xcomplex& z);
  xcomplex& operator%= (int n);

  // Increment and decrement operators. Both prefixed
  // and postfixed versions are defined. These operators
  // only act on the real part of their argument.
  xcomplex& operator++ ();
  xcomplex& operator-- ();
  xcomplex& operator++ (int dummy);
  xcomplex& operator-- (int dummy);

  // Destructor. You will never have to recall it
  // explicitly in your code.
  ~xcomplex (void);

  // Integer exponent power. For any extended precision
  // complex number 'z', z(n) is equal to 'z' raised to 'n'.
  xcomplex operator() (int n) const;

  // This is the usual unary minus.
  xcomplex operator-() const;

  // For any extended precision complex number 'z', 
  // !z evaluates to 1 when
  // 'z' is null, else it evaluates to 0.
  int operator!() const;

  // Functions for conversions. z._2dcomplex(), z._2fcomplex(),
  // z._2cxpr() and z._2string() convert the extended
  // precision complex number 'z' in a double precision
  // complex number, in a single precision complex 
  // number, in a structure of type 'cxpr', and in a
  // string, respectively.
  double_complex _2dcomplex () const;
  float_complex _2fcomplex() const;
  struct cxpr _2cxpr() const;
  string _2string() const;

  // For any extended precision complex number 'z',
  // z.real() and z.imag() return, respectively, the
  // real and the imaginary part of 'z' in the form
  // of an extended precision number.
  xreal real () const;
  xreal imag () const;

  // For any extended precision complex number 'z',
  // z._real() and z._imag() return, respectively, the
  // real and the imaginary part of 'z' in the form
  // of a structure of 'xpr' type.
  struct xpr _real () const;
  struct xpr _imag () const;

  // For any extended precision complex number 'z',
  // z.dreal() and z.dimag() return, respectively, the
  // real and the imaginary part of 'z' in the form
  // of a double precision number.
  double dreal () const;
  double dimag () const;

  // For any extended precision complex number 'z',
  // z.freal() and z.fimag() return, respectively, the
  // real and the imaginary part of 'z' in the form
  // of a single precision number.
  double freal () const;
  double fimag () const;

  // For any extended precision complex number 'z',
  // z.sreal() and z.simag() return, respectively, the
  // real and the imaginary part of 'z' in the form
  // of a string.
  string sreal () const;
  string simag () const;
  
  // The next functions allow to set (or reset)
  // the real and the imaginary part of a complex number.
  void real (const xreal& x);
  void imag (const xreal& x);
  void real (struct xpr x);
  void imag (struct xpr x);
  void real (const struct xpr* px);
  void imag (const struct xpr* px);
  void real (double x);
  void imag (double x);
  void real (float x);
  void imag (float x);
  void real (int n);
  void imag (int n);
  void real (long n);
  void imag (long n);
  void real (unsigned int u);
  void imag (unsigned int u);
  void real (unsigned long u);
  void imag (unsigned long u);
  void real (const char* str, char** endptr = 0);
  void imag (const char* str, char** endptr = 0);
  void real (string str);
  void imag (string str);
  
  // The member function xcomplex::getfrom() can be used
  // to recover an extended precision complex number from an 
  // input stream. The input stream is passed as argument to
  // the function.
  // The return value is 0 in case of input error (in case of 
  // End-Of-File, for example). 
  // When it starts to process its input, this function drops
  // all eventual leading white spaces.
  // After reading the first non space character, it continues
  // to read from the input stream until it finds a white 
  // space or reaches the End-Of-File. 
  // Then it tries to convert into an extended
  // precision complex number the (initial portion of the) 
  // string which has just been read from the input stream.
  // If no conversion can be performed, then  z.getfrom(is)
  // sets 'z' to the value xNAN + xNANi.
  // If the exactly converted value would cause overflow in
  // the real or/and in the imaginary part, then the real part
  // or/and the imaginary part of 'z' are set to xINF or x_INF, 
  // according to the signs of the correct value.
  int getfrom (istream& is);

  // The member function xcomplex::print() can be used to
  // write an extended precision complex number to an output
  // stream. The output stream is passed to the function as 
  // first argument. The next three arguments have the same 
  // meanings of the fields 'notat', 'sf'
  // and 'lim' of the structure 'xoutflags', respectively (see section 
  // "Real Arithmetic").
  int print (ostream& os, int sc_not, int sign, int lim) const;

  // The function call  z.asprint(sc_not, sign, lim)  returns
  // a buffer of characters with the representation, 
  // in form of a decimal ASCII string,
  // of the extended precision complex number 'z'. 
  // The arguments 'sc_not', 'sign' and 'lim' are used 
  // to format the string.
  // They have the same meanings of the fields 'notat', 'sf'
  // and 'lim' of the structure 'xoutflags', respectively (see section 
  // "Real Arithmetic").
  // The buffer returned by this function is malloc'ed inside
  // the function. In case of insufficient memory, the null 
  // pointer is returned.
  char* asprint (int sc_not, int sign, int lim) const;

  // The following static functions are used to set
  // or get the values of the fields of the structure
  // 'xcomplex::ioflags'. This structure is a static member
  // variable of the class 'xcomplex' and it is used by 
  // the output operator << to know how to format its second
  // argument. The meaning of the
  // fields of the structure 'xcomplex::ioflags' is explained
  // in the section "Real arithmetic".

  // xcomplex::set_fmt (which) sets to 'which' the value
  // of 'xcomplex::ioflags.fmt' .
  static void set_fmt (short format);

  // xcomplex::set_notation (which) sets to 'which' the value
  // of 'xcomplex::ioflags.notat' .
  static void set_notation (short notat);

  // xcomplex::set_signflag (which) sets to 'which' the value
  // of 'xcomplex::ioflags.sf' .
  static void set_signflag (short onoff);

  // xcomplex::set_mfwd (which) sets to 'which' the value
  // of 'xcomplex::ioflags.mfwd' .
  static void set_mfwd (short wd);

  // xcomplex::set_lim (which) sets to 'which' the value
  // of 'xcomplex::ioflags.lim' .
  static void set_lim (short lim);

  // xcomplex::set_padding (which) sets to 'which' the value
  // of 'xcomplex::ioflags.padding' .
  static void set_padding (signed char ch);

  // xcomplex::set_ldelim (ch) sets to 'ch' the value
  // of 'xcomplex::ioflags.ldel' .
  static void set_ldelim (signed char ch);

  // xcomplex::set_rdelim (ch) sets to 'ch' the value
  // of 'xcomplex::ioflags.rdel' .
  static void set_rdelim (signed char ch);

  // xcomplex::get_fmt () returns the current value
  // of 'xcomplex::ioflags.fmt' .
  static short get_fmt (void);

  // xcomplex::get_notation () returns the current value
  // of 'xcomplex::ioflags.notat' .
  static short get_notation (void);

  // xcomplex::get_signflag () returns the current value
  // of 'xcomplex::ioflags.sf' .
  static short get_signflag (void);

  // xcomplex::get_mfwd () returns the current value
  // of 'xcomplex::ioflags.mfwd' .
  static short get_mfwd (void);

  // xcomplex::get_lim () returns the current value
  // of 'xcomplex::ioflags.lim' .
  static short get_lim (void);

  // xcomplex::get_padding () returns the current value
  // of 'xcomplex::ioflags.padding' .
  static signed char get_padding (void);

  // xcomplex::get_ldelim () returns the current value
  // of 'xcomplex::ioflags.ldel' .
  static signed char get_ldelim (void);

  // xcomplex::get_rdelim () returns the current value
  // of 'xcomplex::ioflags.rdel' .
  static signed char get_rdelim (void);
 private:
  struct cxpr br; /* binary representation */
  static struct xoutflags ioflags; /* output flags */
};

 // Some useful constants:
 // cxZERO    ==  0
 // cxONE     ==  1
 // cxI       ==  1i 
extern const xcomplex cxZERO, cxONE, cxI;

} /* End namespace HPA */

#define xi cxI
#define xj cxI
#define _i cxI
#define _j cxI

#endif /* _XCOMPLEX_H_ */
