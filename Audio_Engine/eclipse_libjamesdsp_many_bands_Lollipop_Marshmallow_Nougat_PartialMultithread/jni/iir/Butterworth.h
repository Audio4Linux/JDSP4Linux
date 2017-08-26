#ifndef DSPFILTERS_BUTTERWORTH_H
#define DSPFILTERS_BUTTERWORTH_H
#include "Common.h"
#include "Cascade.h"
#include "PoleFilter.h"
#include "State.h"
namespace Iir
{
namespace Butterworth
{
class AnalogLowPass : public LayoutBase
{
public:
    AnalogLowPass ();
    void design (const int numPoles);
private:
    int m_numPoles;
};
class AnalogLowShelf : public LayoutBase
{
public:
    AnalogLowShelf ();
    void design (int numPoles, double gainDb);
private:
    int m_numPoles;
    double m_gainDb;
};
struct LowShelfBase : PoleFilterBase <AnalogLowShelf>
{
    void setup (int order,
                double sampleRate,
                double cutoffFrequency,
                double gainDb);
};
struct HighShelfBase : PoleFilterBase <AnalogLowShelf>
{
    void setup (int order,
                double sampleRate,
                double cutoffFrequency,
                double gainDb);
};
struct BandShelfBase : PoleFilterBase <AnalogLowShelf>
{
    void setup (int order,
                double sampleRate,
                double centerFrequency,
                double widthFrequency,
                double gainDb);
};
template <int MaxOrder, class StateType = DEFAULT_STATE>
struct LowShelf : PoleFilter <LowShelfBase, StateType, MaxOrder>
{
};
template <int MaxOrder, class StateType = DEFAULT_STATE>
struct HighShelf : PoleFilter <HighShelfBase, StateType, MaxOrder>
{
};
template <int MaxOrder, class StateType = DEFAULT_STATE>
struct BandShelf : PoleFilter <BandShelfBase, StateType, MaxOrder, MaxOrder*2>
{
};
}
}
#endif
