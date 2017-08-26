#ifndef DSPFILTERS_POLEFILTER_H
#define DSPFILTERS_POLEFILTER_H
#include "Common.h"
#include "MathSupplement.h"
#include "Cascade.h"
#include "State.h"
namespace Iir
{
class PoleFilterBase2 : public Cascade
{
public:
#if 1
    std::vector<PoleZeroPair> getPoleZeros () const
    {
        std::vector<PoleZeroPair> vpz;
        const int pairs = (m_digitalProto.getNumPoles () + 1) / 2;
        for (int i = 0; i < pairs; ++i)
            vpz.push_back (m_digitalProto[i]);
        return vpz;
    }
#endif
protected:
    LayoutBase m_digitalProto;
};
template <class AnalogPrototype>
class PoleFilterBase : public PoleFilterBase2
{
protected:
    void setPrototypeStorage (const LayoutBase& analogStorage,
                              const LayoutBase& digitalStorage)
    {
        m_analogProto.setStorage (analogStorage);
        m_digitalProto = digitalStorage;
    }
protected:
    AnalogPrototype m_analogProto;
};
template <class BaseClass,
          class StateType,
          int MaxAnalogPoles,
          int MaxDigitalPoles = MaxAnalogPoles>
struct PoleFilter : BaseClass
, CascadeStages <(MaxDigitalPoles + 1) / 2, StateType>
{
    PoleFilter ()
    {
        BaseClass::setCascadeStorage (this->getCascadeStorage());
        BaseClass::setPrototypeStorage (m_analogStorage, m_digitalStorage);
    }
private:
    Layout <MaxAnalogPoles> m_analogStorage;
    Layout <MaxDigitalPoles> m_digitalStorage;
};
class LowPassTransform
{
public:
    LowPassTransform (double fc,
                      LayoutBase& digital,
                      LayoutBase const& analog);
private:
    complex_t transform (complex_t c);
    double f;
};
class HighPassTransform
{
public:
    HighPassTransform (double fc,
                       LayoutBase& digital,
                       LayoutBase const& analog);
private:
    complex_t transform (complex_t c);
    double f;
};
class BandPassTransform
{
public:
    BandPassTransform (double fc,
                       double fw,
                       LayoutBase& digital,
                       LayoutBase const& analog);
private:
    ComplexPair transform (complex_t c);
    double wc;
    double wc2;
    double a;
    double b;
    double a2;
    double b2;
    double ab;
    double ab_2;
};
class BandStopTransform
{
public:
    BandStopTransform (double fc,
                       double fw,
                       LayoutBase& digital,
                       LayoutBase const& analog);
private:
    ComplexPair transform (complex_t c);
    double wc;
    double wc2;
    double a;
    double b;
    double a2;
    double b2;
};
}
#endif
