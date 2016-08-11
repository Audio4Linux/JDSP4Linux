/*******************************************************************************

"A Collection of Useful C++ Classes for Digital Signal Processing"
 By Vinnie Falco adapted for Linux by Bernd Porr

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

#ifndef DSPFILTERS_ELLIPTIC_H
#define DSPFILTERS_ELLIPTIC_H

#include "Common.h"
#include "Cascade.h"
#include "PoleFilter.h"
#include "State.h"

namespace Iir {

/*
 * Filters with Elliptic response characteristics
 *
 */

namespace Elliptic {

// Solves for Jacobi elliptics
class Solver
{
public:
  static double ellipticK (double k);
};

// Half-band analog prototype (s-plane)

class AnalogLowPass : public LayoutBase
{
public:
  AnalogLowPass ();

  void design (const int numPoles,
               double rippleDb,
               double rolloff);

private:
  void prodpoly    (int sn);
  void calcfz2     (int i);
  void calcfz      ();
  void calcqz      ();
  double findfact	 (int t);
  double calcsn		 (double u);

#if 0
  template<int n>
  struct CalcArray
  {
    double& operator[](size_t index)
    {
      assert( index<n );
      return m_a[index];
    }
  private:
    double m_a[n];
  };
#else
#endif

  double m_p0;
  double m_q;
  double m_K;
  double m_Kprime;
  double m_e;
  int m_nin;
  int m_m;
  int m_n2;
  int m_em;
  double m_zeros[100];
  double m_c1[100];
  double m_b1[100];
  double m_a1[100];
  double m_d1[100];
  double m_q1[100];
  double m_z1[100];
  double m_f1[100];
  double m_s1[100];
  double m_p [100];
  double m_zw1[100];
  double m_zf1[100];
  double m_zq1[100];
  double m_rootR[100];
  double m_rootI[100];

  int m_numPoles;
  double m_rippleDb;
  double m_rolloff;
};

//------------------------------------------------------------------------------

// Factored implementations to reduce template instantiations

struct LowPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double rippleDb,
              double rolloff);
};

struct HighPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double cutoffFrequency,
              double rippleDb,
              double rolloff);
};

struct BandPassBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              double rippleDb,
              double rolloff);
};

struct BandStopBase : PoleFilterBase <AnalogLowPass>
{
  void setup (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency,
              double rippleDb,
              double rolloff);
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

}

}

#endif

