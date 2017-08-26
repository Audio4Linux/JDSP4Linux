#ifndef DSPFILTERS_MATHSUPPLEMENT_H
#define DSPFILTERS_MATHSUPPLEMENT_H
#include "Common.h"
namespace Iir
{
const double doublePi =3.1415926535897932384626433832795028841971;
const double doublePi_2 =1.5707963267948966192313216916397514420986;
typedef std::complex<double> complex_t;
typedef std::pair<complex_t, complex_t> complex_pair_t;
inline const complex_t infinity()
{
    return complex_t(std::numeric_limits<double>::infinity());
}
template <typename Ty, typename To>
inline std::complex<Ty> addmul (const std::complex<Ty>& c,
                                Ty v,
                                const std::complex<To>& c1)
{
    return std::complex <Ty> (
               c.real() + v * c1.real(), c.imag() + v * c1.imag());
}
}
#endif
