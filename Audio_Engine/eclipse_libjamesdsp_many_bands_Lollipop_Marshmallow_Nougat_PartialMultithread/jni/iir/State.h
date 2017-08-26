#ifndef DSPFILTERS_STATE_H
#define DSPFILTERS_STATE_H
#include "Common.h"
#include "Biquad.h"
#define DEFAULT_STATE DirectFormII
namespace Iir
{
class DirectFormII
{
public:
    DirectFormII ()
    {
        reset ();
    }
    void reset ()
    {
        m_v1 = 0;
        m_v2 = 0;
    }
    template <typename Sample>
    Sample process1 (const Sample in,
                     const BiquadBase& s)
    {
        double w = in - s.m_a1*m_v1 - s.m_a2*m_v2;
        double out = s.m_b0*w + s.m_b1*m_v1 + s.m_b2*m_v2;
        m_v2 = m_v1;
        m_v1 = w;
        return static_cast<Sample> (out);
    }
private:
    double m_v1;
    double m_v2;
};
}
#endif
