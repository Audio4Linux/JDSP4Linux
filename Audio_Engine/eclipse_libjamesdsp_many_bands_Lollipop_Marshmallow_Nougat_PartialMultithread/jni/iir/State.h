#ifndef DSPFILTERS_STATE_H
#define DSPFILTERS_STATE_H
#include "Common.h"
#include "Biquad.h"
#include <stdexcept>
#define DEFAULT_STATE DirectFormII
namespace Iir
{
class DirectFormI
{
public:
    DirectFormI ()
    {
        reset();
    }
    void reset ()
    {
        m_x1 = 0;
        m_x2 = 0;
        m_y1 = 0;
        m_y2 = 0;
    }
    template <typename Sample>
    inline Sample process1 (const Sample in,
                            const BiquadBase& s)
    {
        double out = s.m_b0*in + s.m_b1*m_x1 + s.m_b2*m_x2
                     - s.m_a1*m_y1 - s.m_a2*m_y2;
        m_x2 = m_x1;
        m_y2 = m_y1;
        m_x1 = in;
        m_y1 = out;
        return static_cast<Sample> (out);
    }
protected:
    double m_x2;
    double m_y2;
    double m_x1;
    double m_y1;
};
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
class TransposedDirectFormII
{
public:
    TransposedDirectFormII ()
    {
        reset ();
    }
    void reset ()
    {
        m_s1 = 0;
        m_s1_1 = 0;
        m_s2 = 0;
        m_s2_1 = 0;
    }
    template <typename Sample>
    inline Sample process1 (const Sample in,
                            const BiquadBase& s)
    {
        double out;
        out = m_s1_1 + s.m_b0*in;
        m_s1 = m_s2_1 + s.m_b1*in - s.m_a1*out;
        m_s2 = s.m_b2*in - s.m_a2*out;
        m_s1_1 = m_s1;
        m_s2_1 = m_s2;
        return static_cast<Sample> (out);
    }
private:
    double m_s1;
    double m_s1_1;
    double m_s2;
    double m_s2_1;
};
}
#endif
