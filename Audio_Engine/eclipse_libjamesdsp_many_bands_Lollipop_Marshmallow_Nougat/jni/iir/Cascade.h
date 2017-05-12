/*******************************************************************************

"A Collection of Useful C++ Classes for Digital Signal Processing"
 By Vinnie Falco and adapted for Linux by Bernd Porr

Official project location:
https://github.com/vinniefalco/DSPFilters

See Documentation.cpp for contact information, notes, and bibliography.

--------------------------------------------------------------------------------

License: MIT License (http://www.opensource.org/licenses/mit-license.php)
Copyright (c) 2009 by Vinnie Falco

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*******************************************************************************/

#ifndef DSPFILTERS_CASCADE_H
#define DSPFILTERS_CASCADE_H

#include "Common.h"
#include "Biquad.h"
#include "Layout.h"
#include "MathSupplement.h"

namespace Iir {

/*
 * Holds coefficients for a cascade of second order sections.
 *
 */

// Factored implementation to reduce template instantiations
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
    assert (index >= 0 && index <= m_numStages);
    return m_stageArray[index];
  }

  // Calculate filter response at the given normalized frequency.
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

//------------------------------------------------------------------------------

// Storage for Cascade
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
