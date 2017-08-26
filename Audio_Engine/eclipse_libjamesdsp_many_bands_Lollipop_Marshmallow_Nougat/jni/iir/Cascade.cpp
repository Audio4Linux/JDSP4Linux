#include "Common.h"
#include "Cascade.h"
namespace Iir
{
Cascade::Cascade ()
    : m_numStages (0)
    , m_maxStages (0)
    , m_stageArray (0)
{
}
void Cascade::setCascadeStorage (const Storage& storage)
{
    m_numStages = 0;
    m_maxStages = storage.maxStages;
    m_stageArray = storage.stageArray;
}
complex_t Cascade::response (double normalizedFrequency) const
{
    double w = 2 * doublePi * normalizedFrequency;
    const complex_t czn1 = std::polar (1., -w);
    const complex_t czn2 = std::polar (1., -2 * w);
    complex_t ch (1);
    complex_t cbot (1);
    const Biquad* stage = m_stageArray;
    for (int i = m_numStages; --i >=0; ++stage)
    {
        complex_t cb (1);
        complex_t ct (stage->getB0()/stage->getA0());
        ct = addmul (ct, stage->getB1()/stage->getA0(), czn1);
        ct = addmul (ct, stage->getB2()/stage->getA0(), czn2);
        cb = addmul (cb, stage->getA1()/stage->getA0(), czn1);
        cb = addmul (cb, stage->getA2()/stage->getA0(), czn2);
        ch *= ct;
        cbot *= cb;
    }
    return ch / cbot;
}
std::vector<PoleZeroPair> Cascade::getPoleZeros () const
{
    std::vector<PoleZeroPair> vpz;
    vpz.reserve (m_numStages);
    const Stage* stage = m_stageArray;
    for (int i = m_numStages; --i >=0;)
    {
        BiquadPoleState bps (*stage++);
        vpz.push_back (bps);
    }
    return vpz;
}
void Cascade::applyScale (double scale)
{
    m_stageArray->applyScale (scale);
}
void Cascade::setLayout (const LayoutBase& proto)
{
    const int numPoles = proto.getNumPoles();
    m_numStages = (numPoles + 1)/ 2;
    Biquad* stage = m_stageArray;
    for (int i = 0; i < m_numStages; ++i, ++stage)
        stage->setPoleZeroPair (proto[i]);
    applyScale (proto.getNormalGain() /
                std::abs (response (proto.getNormalW() / (2 * doublePi))));
}
}
