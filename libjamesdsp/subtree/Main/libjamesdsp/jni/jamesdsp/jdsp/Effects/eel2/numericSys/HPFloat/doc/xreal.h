#ifndef _XREAL_H_
#define _XREAL_H_

#include <xpre.h>
#include <iostream>
#include <string>
#include <cstdlib>

using std::ostream;
using std::istream;
using std::string;

namespace HPA {

class xreal {
  // << and >> are used respectively for the output and the 
  // input of extended precision numbers.
  // The input operator >> reads a double precision
  // number and then converts it to an extended precision 
  // number. This can have undesirable rounding effects. 
  // To avoid them, use the input function
  // xreal::getfrom() (see below).
  friend ostream& operator<< (ostream& os, const xreal& x);
  friend istream& operator>> (istream& is, xreal& x);

  // +, -, *, / are the usual arithmetic operators
  friend xreal operator+ (const xreal& x1, const xreal& x2);
  friend xreal operator- (const xreal& x1, const xreal& x2);
  friend xreal operator* (const xreal& x1, const xreal& x2);
  friend xreal operator/ (const xreal& x1, const xreal& x2);

  // x % n is equal to x * pow (2,n)
  friend xreal operator% (const xreal& x1, int n);

  // ==, !=, <=, >=, <, > are the usual comparison operators
  friend int operator== (const xreal& x1, const xreal& x2);
  friend int operator!= (const xreal& x1, const xreal& x2);
  friend int operator<= (const xreal& x1, const xreal& x2);
  friend int operator>= (const xreal& x1, const xreal& x2);
  friend int operator< (const xreal& x1, const xreal& x2);
  friend int operator> (const xreal& x1, const xreal& x2);

  // sget (s, n, x) tries to read an extended precision
  // number from the string 's' starting from the position
  // 'n'. The retrieved number is converted and stored in
  // 'x'. The return value is the number of characters 
  // composing the decimal representation of this number
  // as read from 's'. For example, if s == "12.34dog" and
  // n == 0, then  'x'  is set to 12.34 and the return value
  // is 5.
  // If the portion of 's' starting from the position 'n'
  // can not be converted to a number, then 'x' is set to
  // xNAN and 0 is returned.
  // If the exactly converted value would cause overflow, 
  // then xINF or x_INF is returned, according to the sign
  // of the value.
  // If 'n' is greater or equal to the length of 's', then 0
  // is returned and 'x' is set to xZERO.
  friend unsigned long sget (string s, unsigned long startptr,
                             xreal& x);

  // bget (buff, x) tries to read an extended precision
  // number from the buffer pointed to by 'buff'. 
  // The retrieved number is converted and stored in 'x'.
  // The return value is a pointer to the character after
  // the last character used in the conversion.
  // For example, if 'buff' is a pointer to the buffer
  // "12.34dog", then 'x' is set to 12.34 and the return 
  // value is a pointer to "dog" (i.e., a pointer
  // to the character 'd').
  // If the initial portion of the string pointed to by 'buff'
  // can not be converted to a number, then 'x' is set to xNAN
  // and 'buff' is returned.
  // If the exactly converted value would cause overflow, 
  // then xINF or x_INF is returned, according to the sign 
  // of the value.
  // If 'buff' is NULL (0), then an error message is printed
  // on 'cerr' (standard error device).
  friend const char* bget (const char* buff, xreal& x);

  // compare (x1, x2) returns
  //+1 to mean x1 >  x2
  // 0 to mean x1 == x2
  //-1 to mean x1 <  x2
  friend int compare (const xreal& x1, const xreal& x2);

  //isNaN (x) returns 1 when x == xNAN, else 0
  friend int isNaN (const xreal& x);

  // The following functions do not need a particular comment:
  // each of them is defined as the corresponding function
  // of the standard math library, that is to say the function
  // from <cmath> having the same name.
  // However qfmod(), sfmod(), frac() and fix() do not have
  // counterparts in the standard math library. 
  // With respect to fmod(), qfmod() requires one more
  // argument, where the quotient of the division of
  // the first argument by the second one is stored.
  // sfmod (x,&n) stores in the integer variable
  // 'n' the integer part of 'x' and, at the same time,
  // returns the fractional part of 'x'. 
  // The usage of sfmod() is strongly discouraged.
  // frac() returns the fractional part of its argument.  
  // Finally, fix() is a frontend to the xfix() 
  // function (see section "Extended Precision Floating
  // Point Arithmetic").
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
  
 public:
  // Various constructors. They allow to define
  // an extended precision number in several ways.
  // In addition, they allow for conversions from other
  // numeric types.
  xreal (const struct xpr* px = &xZero);
  xreal (struct xpr x);
  xreal (double x);
  xreal (float x);
  xreal (int n);
  xreal (long n);
  xreal (unsigned int u);
  xreal (unsigned long u);

  // This constructor requires a special comment. If
  // only the first argument is present, the initial portion
  // of the string pointed to by this argument is converted
  // into an extended precision number, if a conversion is
  // possible. If no conversion is possible, then the 
  // returned number is xNAN. If the second argument is
  // present and is not null, it must be the address of a 
  // valid pointer to 'char'.
  // Before returning, the constructor will set this pointer
  // so that it points to the character of the string 'str' after the last 
  // character used in the conversion.
  xreal (const char* str, char** endptr = 0);
  xreal (string str);
  xreal (const xreal& x);

  // Assignment operators. They do not require
  // any explanation with the only exception of '%=',
  // which combines a '%' operation with an assignment.
  // So, x %= n is equivalent to x *= pow(2,n) .
  xreal& operator= (const xreal& x);
  xreal& operator+= (const xreal& x);
  xreal& operator-= (const xreal& x);
  xreal& operator*= (const xreal& x);
  xreal& operator/= (const xreal& x);
  xreal& operator%= (int n);

  // Increment and decrement operators. Both prefixed
  // and postfixed versions are defined.
  xreal& operator++ ();
  xreal& operator-- ();
  xreal& operator++ (int dummy);
  xreal& operator-- (int dummy);

  // Destructor. You will never have to recall it
  // explicitly in your code.
  ~xreal (void);

  // Integer exponent power. For any extended precision
  // number 'x', x(n) is equal to 'x' raised to 'n'.
  xreal operator() (int n) const;

  // This is the usual unary minus.
  xreal operator-() const;

  // For any extended precision number 'x', !x evaluates to 1
  // when 'x' is null, else it evaluates to 0.
  int operator!() const;

  // x.isneg() returns 1 if 'x' is negative, else it 
  // returns 0.
  int isneg() const;

  // x.exp() returns the exponent part
  // of the binary representation of 'x'.
  int exp() const;

  // Functions for conversions.  x._2double(), x._2float(), 
  // x._2xpr() and x._2string() convert the extended precision
  // number 'x'  in a double precision number, in a single 
  // precision number, in a structure of
  // type 'xpr', and in a string, respectively.
  double _2double () const;
  float _2float() const;
  struct xpr _2xpr() const;
  string _2string() const;

  // The member function xreal::getfrom() can be used to 
  // recover an extended precision number from an input 
  // stream. The input stream is passed as argument to the
  // function. 
  // The return value is 0 in case of input error (in case
  // of End-Of-File, for example). 
  // When it starts to process its input, this function drops
  // all the eventual leading white spaces.
  // After reading the first non space character, it 
  // continues to read from the input stream until it finds
  // a white space or reaches the End-Of-File. 
  // Then it tries to convert into an extended
  // precision number the (initial portion of the) string just
  // read.
  // If no conversion can be performed, then  x.getfrom(is)
  // sets 'x' to the value xNAN.
  // If the exactly converted value would cause overflow,
  // then 'x' is set to xINF or x_INF, according to the sign
  // of the correct value.
  int getfrom (istream& is);

  // The member function xreal::print() can be used to write
  // an extended precision number to an output stream.
  // The output stream is passed to the function as first 
  // argument. The next three arguments have the same meanings
  // of the fields 'notat', 'sf' and 'lim' of the
  // structure 'xoutflags', respectively (see section "Real Arithmetic").
  int print (ostream& os, int sc_not, int sign, int lim) const;

  // The function call  x.asprint(sc_not, sign, lim)  returns
  // a buffer of characters with the representation, in form 
  // of a decimal ASCII string, of the extended precision 
  // number 'x'. The arguments 'sc_not', 'sign' and 'lim' are
  // used to format the string.
  // They have the same meanings of the fields 'notat', 'sf'
  // and 'lim' of the structure 'xoutflags', respectively (see section 
  // "Real Arithmetic").
  // The buffer returned by this function is malloc'ed inside
  // the function. In case of insufficient memory, the null 
  // pointer is returned.
  char* asprint (int sc_not, int sign, int lim) const;

  // The following static functions are used to set
  // or get the values of the fields of the structure
  // 'xreal::ioflags'. This structure is a static member
  // variable of the class 'xreal' and it is used by the
  // output operator << to know how to format its second
  // argument. The meaning of the fields of the structure
  // 'xreal::ioflags' is explained in the section 
  // "Real arithmetic".

  // xreal::set_notation (which) sets to 'which' the value
  // of 'xreal::ioflags.notat' .
  static void set_notation (short notat);

  // xreal::set_signflag (which) sets to 'which' the value
  // of 'xreal::ioflags.sf' .
  static void set_signflag (short onoff);

  // xreal::set_mfwd (which) sets to 'which' the value
  // of 'xreal::ioflags.mfwd' .
  static void set_mfwd (short wd);

  // xreal::set_lim (which) sets to 'which' the value
  // of 'xreal::ioflags.lim' .
  static void set_lim (short lim);

  // xreal::set_padding (which) sets to 'which' the value
  // of 'xreal::ioflags.padding' .
  static void set_padding (signed char ch);

  // xreal::get_notation () returns the current value
  // of 'xreal::ioflags.notat' .
  static short get_notation (void);

  // xreal::get_signflag () returns the current value
  // of 'xreal::ioflags.sf' .
  static short get_signflag (void);

  // xreal::get_mfwd () returns the current value
  // of 'xreal::ioflags.mfwd' .
  static short get_mfwd (void);

  // xreal::get_lim () returns the current value
  // of 'xreal::ioflags.lim' .
  static short get_lim (void);

  // xreal::get_padding () returns the current value
  // of 'xreal::ioflags.padding' .
  static signed char get_padding (void);
 private:
  struct xpr br; /* binary representation */
  static struct xoutflags ioflags; /* output flags */
};

 // xmatherrcode() returns the current value of the global
 // variable 'xErrNo' (see section 
 // "Dealing with runtime errors"), if
 // this variable is defined. 
 // Otherwise xmatherrcode() returns -1. 
 int xmatherrcode ();

 // clear_xmatherr() resets to 0 the value of the global
 // variable 'xErrNo' (see section "Dealing with runtime
 // errors"), if this variable is defined. 
 // Otherwise, clear_xmatherr() prints a suitable warning
 // on 'cerr' (standard error device).
 void clear_xmatherr ();

 // Some useful constants:
 // xZERO    ==  0
 // xONE     ==  1
 // xTWO     ==  2
 // xTEN     == 10
 // xINF     == +INF
 // x_INF    == -INF
 // xNAN     == Not-A-Number
 // xPI      == Pi Greek
 // xPI2     == Pi / 2
 // xPI4     == Pi / 4
 // xEE      == e (base of natural logarithms)
 // xSQRT2   == square root of 2
 // xLN2     == natural logarithm of 2
 // xLN10    == natural logarithm of 10
 // xLOG2_E  == base-2 logarithm of e
 // xLOG2_10 == base-2 logarithm of 10 
 // xLOG10_E == base-10 logarithm of e
 extern const xreal xZERO, xONE, xTWO, xTEN;
 extern const xreal xINF, x_INF, xNAN;
 extern const xreal xPI, xPI2, xPI4, xEE, xSQRT2;
 extern const xreal xLN2, xLN10, xLOG2_E, xLOG2_10, xLOG10_E;

} /* End namespace HPA */

#endif /* _XREAL_H_ */
