#ifndef DSPFILTERS_BIQUAD_H
#define DSPFILTERS_BIQUAD_H
#include "Common.h"
#include "MathSupplement.h"
#include "Types.h"
namespace Iir
{
struct BiquadPoleState;
class BiquadBase
{
public:
    template <class StateType>
    struct State : StateType
    {
        template <typename Sample>
        inline Sample process (const Sample in, const BiquadBase& b)
        {
            return static_cast<Sample> (StateType::process1 (in, b));
        }
    };
public:
    std::vector<PoleZeroPair> getPoleZeros () const;
    double getA0 () const
    {
        return m_a0;
    }
    double getA1 () const
    {
        return m_a1*m_a0;
    }
    double getA2 () const
    {
        return m_a2*m_a0;
    }
    double getB0 () const
    {
        return m_b0*m_a0;
    }
    double getB1 () const
    {
        return m_b1*m_a0;
    }
    double getB2 () const
    {
        return m_b2*m_a0;
    }
    template <class StateType, typename Sample>
    Sample filter(Sample s, StateType& state) const
    {
        return state.process (s, *this);
    }
protected:
    void setCoefficients (double a0, double a1, double a2,
                          double b0, double b1, double b2);
    void setOnePole (complex_t pole, complex_t zero);
    void setTwoPole (complex_t pole1, complex_t zero1,
                     complex_t pole2, complex_t zero2);
    void setPoleZeroPair (const PoleZeroPair& pair)
    {
        if (pair.isSinglePole ())
            setOnePole (pair.poles.first, pair.zeros.first);
        else
            setTwoPole (pair.poles.first, pair.zeros.first,
                        pair.poles.second, pair.zeros.second);
    }
    void setPoleZeroForm (const BiquadPoleState& bps);
    void applyScale (double scale);
public:
    double m_a0;
    double m_a1;
    double m_a2;
    double m_b1;
    double m_b2;
    double m_b0;
};
struct BiquadPoleState : PoleZeroPair
{
    BiquadPoleState () { }
    explicit BiquadPoleState (const BiquadBase& s);
    double gain;
};
class Biquad : public BiquadBase
{
public:
    Biquad ();
    explicit Biquad (const BiquadPoleState& bps);
public:
    void setOnePole (complex_t pole, complex_t zero)
    {
        BiquadBase::setOnePole (pole, zero);
    }
    void setTwoPole (complex_t pole1, complex_t zero1,
                     complex_t pole2, complex_t zero2)
    {
        BiquadBase::setTwoPole (pole1, zero1, pole2, zero2);
    }
    void setPoleZeroPair (const PoleZeroPair& pair)
    {
        BiquadBase::setPoleZeroPair (pair);
    }
    void applyScale (double scale)
    {
        BiquadBase::applyScale (scale);
    }
};
}
#endif
