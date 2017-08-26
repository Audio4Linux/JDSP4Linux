#include "Common.h"
#include "MathSupplement.h"
#include "Biquad.h"
namespace Iir
{
BiquadPoleState::BiquadPoleState (const BiquadBase& s)
{
    const double a0 = s.getA0 ();
    const double a1 = s.getA1 ();
    const double a2 = s.getA2 ();
    const double b0 = s.getB0 ();
    const double b1 = s.getB1 ();
    const double b2 = s.getB2 ();
    if (a2 == 0 && b2 == 0)
    {
        poles.first = -a1;
        zeros.first = -b0 / b1;
        poles.second = 0;
        zeros.second = 0;
    }
    else
    {
        {
            const complex_t c = sqrt (complex_t (a1 * a1 - 4 * a0 * a2, 0));
            double d = 2. * a0;
            poles.first = -(a1 + c) / d;
            poles.second = (c - a1) / d;
        }
        {
            const complex_t c = sqrt (complex_t (b1 * b1 - 4 * b0 * b2, 0));
            double d = 2. * b0;
            zeros.first = -(b1 + c) / d;
            zeros.second = (c - b1) / d;
        }
    }
    gain = b0 / a0;
}
std::vector<PoleZeroPair> BiquadBase::getPoleZeros () const
{
    std::vector<PoleZeroPair> vpz;
    BiquadPoleState bps (*this);
    vpz.push_back (bps);
    return vpz;
}
void BiquadBase::setCoefficients (double a0, double a1, double a2,
                                  double b0, double b1, double b2)
{
    m_a0 = a0;
    m_a1 = a1/a0;
    m_a2 = a2/a0;
    m_b0 = b0/a0;
    m_b1 = b1/a0;
    m_b2 = b2/a0;
}
void BiquadBase::setOnePole (complex_t pole, complex_t zero)
{
    const double a0 = 1;
    const double a1 = -pole.real();
    const double a2 = 0;
    const double b0 = -zero.real();
    const double b1 = 1;
    const double b2 = 0;
    setCoefficients (a0, a1, a2, b0, b1, b2);
}
void BiquadBase::setTwoPole (complex_t pole1, complex_t zero1,
                             complex_t pole2, complex_t zero2)
{
    const double a0 = 1;
    double a1;
    double a2;
    if (pole1.imag() != 0)
    {
        a1 = -2 * pole1.real();
        a2 = std::norm (pole1);
    }
    else
    {
        a1 = -(pole1.real() + pole2.real());
        a2 = pole1.real() * pole2.real();
    }
    const double b0 = 1;
    double b1;
    double b2;
    if (zero1.imag() != 0)
    {
        b1 = -2 * zero1.real();
        b2 = std::norm (zero1);
    }
    else
    {
        b1 = -(zero1.real() + zero2.real());
        b2 = zero1.real() * zero2.real();
    }
    setCoefficients (a0, a1, a2, b0, b1, b2);
}
void BiquadBase::setPoleZeroForm (const BiquadPoleState& bps)
{
    setPoleZeroPair (bps);
    applyScale (bps.gain);
}
void BiquadBase::applyScale (double scale)
{
    m_b0 *= scale;
    m_b1 *= scale;
    m_b2 *= scale;
}
Biquad::Biquad ()
{
}
Biquad::Biquad (const BiquadPoleState& bps)
{
    setPoleZeroForm (bps);
}
}
