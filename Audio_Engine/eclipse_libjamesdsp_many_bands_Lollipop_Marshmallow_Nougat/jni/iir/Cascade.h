#ifndef DSPFILTERS_CASCADE_H
#define DSPFILTERS_CASCADE_H
#include "Common.h"
#include "Biquad.h"
#include "Layout.h"
#include "MathSupplement.h"
namespace Iir
{
class Cascade
{
public:
    struct Stage : Biquad
    {
    };
    struct Storage
    {
        Storage (int maxStages_, Stage* stageArray_)
            : maxStages (maxStages_)
            , stageArray (stageArray_)
        {
        }
        int maxStages;
        Stage* stageArray;
    };
    int getNumStages () const
    {
        return m_numStages;
    }
    const Stage& operator[] (int index)
    {
        return m_stageArray[index];
    }
    complex_t response (double normalizedFrequency) const;
    std::vector<PoleZeroPair> getPoleZeros () const;
protected:
    Cascade ();
    void setCascadeStorage (const Storage& storage);
    void applyScale (double scale);
    void setLayout (const LayoutBase& proto);
private:
    int m_numStages;
    int m_maxStages;
    Stage* m_stageArray;
};
template <int MaxStages,class StateType>
class CascadeStages
{
public:
    void reset ()
    {
        StateType* state = m_states;
        for (int i = MaxStages; --i >= 0; ++state)
            state->reset();
    }
public:
    template <typename Sample>
    inline Sample filter(const Sample in)
    {
        double out = in;
        StateType* state = m_states;
        Biquad const* stage = m_stages;
        for (int i = MaxStages; --i >= 0; ++state, ++stage)
            out = state->process1 (out, *stage);
        return static_cast<Sample> (out);
    }
    Cascade::Storage getCascadeStorage()
    {
        return Cascade::Storage (MaxStages, m_stages);
    }
private:
    Cascade::Stage m_stages[MaxStages];
    StateType m_states[MaxStages];
};
}
#endif
