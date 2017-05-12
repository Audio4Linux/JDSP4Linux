/*******************************************************************************

"A Collection of Useful C++ Classes for Digital Signal Processing"
 By Vinnie Falco adapted by Bernd Porr for Unix

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

#ifndef DSPFILTERS_BIQUAD_H
#define DSPFILTERS_BIQUAD_H

#include "Common.h"
#include "MathSupplement.h"
#include "Types.h"

namespace Iir {

struct BiquadPoleState;

/*
 * Holds coefficients for a second order Infinite Impulse Response
 * digital filter. This is the building block for all IIR filters.
 *
 */

// Factored interface to prevent outsiders from fiddling
class BiquadBase
{
public:
  template <class StateType>
  struct State : StateType
  {
    template <typename Sample>
    inline Sample process (const Sample in, const BiquadBase& b)
    {
      return static_cast<Sample> (StateType::process1 (in, b));
    }
  };

public:
  // Calculate filter response at the given normalized frequency.
  complex_t response (double normalizedFrequency) const;

  std::vector<PoleZeroPair> getPoleZeros () const;

  double getA0 () const { return m_a0; }
  double getA1 () const { return m_a1*m_a0; }
  double getA2 () const { return m_a2*m_a0; }
  double getB0 () const { return m_b0*m_a0; }
  double getB1 () const { return m_b1*m_a0; }
  double getB2 () const { return m_b2*m_a0; }

  // Process a sample in the given form
  template <class StateType, typename Sample>
  Sample filter(Sample s, StateType& state) const
  {
	  return state.process (s, *this);
  }

protected:
  //
  // These are protected so you can't mess with RBJ biquads
  //

  void setCoefficients (double a0, double a1, double a2,
                        double b0, double b1, double b2);

  void setOnePole (complex_t pole, complex_t zero);

  void setTwoPole (complex_t pole1, complex_t zero1,
                   complex_t pole2, complex_t zero2);

  void setPoleZeroPair (const PoleZeroPair& pair)
  {
    if (pair.isSinglePole ())
      setOnePole (pair.poles.first, pair.zeros.first);
    else
      setTwoPole (pair.poles.first, pair.zeros.first,
                  pair.poles.second, pair.zeros.second);
  }

  void setPoleZeroForm (const BiquadPoleState& bps);

  void setIdentity ();

  void applyScale (double scale);

public:
  double m_a0;
  double m_a1;
  double m_a2;
  double m_b1;
  double m_b2;
  double m_b0;
};

//------------------------------------------------------------------------------

// Expresses a biquad as a pair of pole/zeros, with gain
// values so that the coefficients can be reconstructed precisely.
struct BiquadPoleState : PoleZeroPair
{
  BiquadPoleState () { }

  explicit BiquadPoleState (const BiquadBase& s);

  double gain;
};

// More permissive interface for fooling around
class Biquad : public BiquadBase
{
public:
  Biquad ();

  explicit Biquad (const BiquadPoleState& bps);

public:
  // Export these as public

  void setOnePole (complex_t pole, complex_t zero)
  {
    BiquadBase::setOnePole (pole, zero);
  }

  void setTwoPole (complex_t pole1, complex_t zero1,
                   complex_t pole2, complex_t zero2)
  {
    BiquadBase::setTwoPole (pole1, zero1, pole2, zero2);
  }

  void setPoleZeroPair (const PoleZeroPair& pair)
  {
    BiquadBase::setPoleZeroPair (pair);
  }

  void applyScale (double scale)
  {
    BiquadBase::applyScale (scale);
  }
};

}

#endif
