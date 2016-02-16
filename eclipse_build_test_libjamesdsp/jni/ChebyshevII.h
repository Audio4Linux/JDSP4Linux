/*******************************************************************************

"A Collection of Useful C++ Classes for Digital Signal Processing"
 By Vinnie Falco and adapted by Bernd Porr

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

#ifndef DSPFILTERS_CHEBYSHEVII_H
#define DSPFILTERS_CHEBYSHEVII_H

#include "Common.h"
#include "Cascade.h"
#include "PoleFilter.h"
#include "State.h"

namespace Iir {

/*
 * Filters with Inverse Chebyshev response characteristics
 *
 */

namespace ChebyshevII {

// Half-band analog prototypes (s-plane)

class AnalogLowPass : public LayoutBase
{
public:
  AnalogLowPass ();

  void design (const int numPoles,
               double stopBandDb);

private:
  int m_numPoles;
  double m_stopBandDb;
};

//------------------------------------------------------------------------------

class AnalogLowShelf : public LayoutBase
{
public:
  AnalogLowShelf ();

  void design (int numPoles,
               double gainDb,
               double stopBandDb);

private:
  int m_numPoles;
  double m_stopBandDb;
  double m_gainDb;
};

//------------------------------------------------------------------------------

// Factored implementations to reduce template instantiations

struct LowPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double stopBandDb);
};

struct HighPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double stopBandDb);
};

struct BandPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              double stopBandDb);
};

struct BandStopBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              double stopBandDb);
};

struct LowShelfBase : PoleFilterBase <AnalogLowShelf>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double gainDb,
              double stopBandDb);
};

struct HighShelfBase : PoleFilterBase <AnalogLowShelf>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double gainDb,
              double stopBandDb);
};

struct BandShelfBase : PoleFilterBase <AnalogLowShelf>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              double gainDb,
              double stopBandDb);
};

//------------------------------------------------------------------------------

//
// Raw filters
//

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct LowPass : PoleFilter <LowPassBase, StateType, MaxOrder>
{
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct HighPass : PoleFilter <HighPassBase, StateType, MaxOrder>
{
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct BandPass : PoleFilter <BandPassBase, StateType, MaxOrder, MaxOrder*2>
{
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct BandStop : PoleFilter <BandStopBase, StateType, MaxOrder, MaxOrder*2>
{
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

