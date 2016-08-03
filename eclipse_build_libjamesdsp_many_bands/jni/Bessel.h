/*******************************************************************************

"A Collection of Useful C++ Classes for Digital Signal Processing"
 By Vinnie Falco and adapted for Unix by Bernd Porr

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

#ifndef DSPFILTERS_BESSEL_H
#define DSPFILTERS_BESSEL_H

#include "Common.h"
#include "Cascade.h"
#include "PoleFilter.h"
#include "RootFinder.h"
#include "State.h"

namespace Iir {

/*
 * Filters with Bessel response characteristics
 *
 */

namespace Bessel {

// A Workspace is necessary to find roots

struct WorkspaceBase
{
  WorkspaceBase (RootFinderBase* rootsBase)
    : roots (*rootsBase)
  {
  }

  RootFinderBase& roots;

private:
  WorkspaceBase (WorkspaceBase&);
  WorkspaceBase& operator= (WorkspaceBase&);
};

template <int MaxOrder>
struct Workspace : WorkspaceBase
{
  Workspace ()
    : WorkspaceBase (&m_roots)
  {
  }

private:
  RootFinder <MaxOrder> m_roots;
};

//------------------------------------------------------------------------------

// Half-band analog prototypes (s-plane)

class AnalogLowPass : public LayoutBase
{
public:
  AnalogLowPass ();

  void design (const int numPoles,
               WorkspaceBase* w);

private:
  int m_numPoles;
};

//------------------------------------------------------------------------------

class AnalogLowShelf : public LayoutBase
{
public:
  AnalogLowShelf ();

  void design (int numPoles,
               double gainDb,
               WorkspaceBase* w);

private:
  int m_numPoles;
  double m_gainDb;
};

//------------------------------------------------------------------------------

// Factored implementations to reduce template instantiations

struct LowPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              WorkspaceBase* w);
};

struct HighPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              WorkspaceBase* w);
};

struct BandPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              WorkspaceBase* w);
};

struct BandStopBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              WorkspaceBase* w);
};

struct LowShelfBase : PoleFilterBase <AnalogLowShelf>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double gainDb,
              WorkspaceBase* w);
};

//------------------------------------------------------------------------------

//
// Raw filters
//

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct LowPass : PoleFilter <LowPassBase, StateType, MaxOrder>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency)
  {
    Workspace <MaxOrder> w;
    LowPassBase::setup (order,
                        sampleRate,
                        cutoffFrequency,
                        &w);
  }
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct HighPass : PoleFilter <HighPassBase, StateType, MaxOrder>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency)
  {
    Workspace <MaxOrder> w;
    HighPassBase::setup (order,
                         sampleRate,
                         cutoffFrequency,
                         &w);
  }
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct BandPass : PoleFilter <BandPassBase, StateType, MaxOrder, MaxOrder*2>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency)
  {
    Workspace <MaxOrder> w;
    BandPassBase::setup (order,
                         sampleRate,
                         centerFrequency,
                         widthFrequency,
                         &w);
  }
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct BandStop : PoleFilter <BandStopBase, StateType, MaxOrder, MaxOrder*2>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency)
  {
    Workspace <MaxOrder> w;
    BandStopBase::setup (order,
                         sampleRate,
                         centerFrequency,
                         widthFrequency,
                         &w);
  }
};

template <int MaxOrder, class StateType = DEFAULT_STATE>
struct LowShelf : PoleFilter <LowShelfBase, StateType, MaxOrder, MaxOrder*2>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double gainDb)
  {
    Workspace <MaxOrder> w;
    LowShelfBase::setup (order,
                         sampleRate,
                         cutoffFrequency,
                         gainDb,
                         &w);
  }
};

}

}

#endif

/* This is a test of svn:external */
