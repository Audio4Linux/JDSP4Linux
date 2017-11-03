#include "Butterworth.h"
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void shelfReset(IIRCoefficient *coeffs, BiquadState *stateVar)
{
	stateVar->_xn1 = 0;
	stateVar->_xn2 = 0;
	stateVar->xn3 = 0;
	stateVar->xn4 = 0;
	for (int i = 0; i < coeffs->numFilters; i++)
	{
		stateVar->_yn[i] = 0;
		stateVar->_yn1[i] = 0;
		stateVar->_yn2[i] = 0;
		if (stateVar->_yn3)
		{
			stateVar->_yn3[i] = 0;
			stateVar->_yn4[i] = 0;
		}
	}
}
//******************************************************************************
// High-Order Equalizer Filters: Low-Shelf, High-Shelf *Parametric Boost-Cut
//
// Reference: Sophocles J. Orfanidis, "High-Order Digital Parametric Equalizer
//            Design," J. Audio Eng. Soc., vol.53, pp. 1026-1046, November 2005.
//            http://eceweb1.rutgers.edu/~orfanidi/hpeq/
////******************************************************************************
int shelfCoefficientsGen(IIRCoefficient *coeffs, BiquadState *stateVar, unsigned int filter, double fs, double f1, double f2, unsigned int filterOrder, double overallGain)
{
	if (f1 > fs / 2)
		f1 = fs / 2;
	if (f2 > fs / 2)
		f2 = fs / 2;
	if (f1 < 0)
		f1 = 0;
	if (f2 < 0)
		f2 = 0;
	// Convert band edges to radians/second
	const double pi_2 = 2.0 * M_PI;
	double w1 = pi_2 * (f1 / fs);
	double w2 = pi_2 * (f2 / fs);
	// Compute center frequency w0 *bandwidth Dw
	// for parametric case in radians/sample
	double Dw = w2 - w1;
	double w0 = acos(sin(w1 + w2) / (sin(w1) + sin(w2)));
	if (w2 == M_PI)
		w0 = M_PI;
	// Setup gain
	double G0 = 1.0;        // Reference gain == 0 dB
	coeffs->filterType = filter;
	unsigned int L = (int)ceil(filterOrder*0.5);
	if (filterOrder < 2)
	{
		L = 1;
		overallGain /= 2;
	}
	double G;
	if (overallGain < 0.0)
		G = overallGain - 0.00000000000005;
	else
		G = overallGain + 0.00000000000005;
	double GB = 0.75 * G;   // Setup Peak-to-Bandwidth Gain Ratio. What about the 3-dB point??
	G = pow(10, (G / 20.0));     // G  = 10^(G/20);
	GB = pow(10, (GB / 20.0));    // GB = 10^(GB/20);
								  //  Do not proceed with design if G == G0
	double c0 = cos(w0);
	if (w0 == 0)
		c0 = 1.0;
	if (w0 == M_PI / 2)
		c0 = 0.0;
	if (w0 == M_PI)
		c0 = -1.0;
	double WB = tan(Dw / 2.0);
	double epsilon = sqrt(((G * G) - (GB * GB)) / ((GB * GB) - (G0 * G0)));
	double g = pow(G, (1.0 / filterOrder));
	double g0 = pow(G0, (1.0 / filterOrder));
	double b = WB / pow(epsilon, (1.0 / filterOrder));
	if (coeffs->numFilters != L)
		shelfDeallocateCoeffs(coeffs);
	if (!coeffs->b0)
	{
		size_t size = L * sizeof(double);
		coeffs->b0 = (double*)malloc(size);
		coeffs->b1 = (double*)malloc(size);
		coeffs->b2 = (double*)malloc(size);
		coeffs->a1 = (double*)malloc(size);
		coeffs->a2 = (double*)malloc(size);
		if (filter == 2)
		{
			coeffs->b3 = (double*)malloc(size);
			coeffs->b4 = (double*)malloc(size);
			coeffs->a3 = (double*)malloc(size);
			coeffs->a4 = (double*)malloc(size);
			if (!(coeffs->b3 || coeffs->b4 || coeffs->a3 || coeffs->a4))
				return -1;
			coeffs->process = &shelfProcessFourthOrderSections;
		}
		else
		{
			coeffs->b3 = 0;
			coeffs->b4 = 0;
			coeffs->a3 = 0;
			coeffs->a4 = 0;
			coeffs->process = &shelfProcessBiquad;
		}
		if (!(coeffs->b0 || coeffs->b1 || coeffs->b2 || coeffs->a1 || coeffs->a2))
			return -1;
		coeffs->numFilters = L;
	}
	unsigned int i;
	for (i = 1; i <= L; i++)
	{
		double phi = (2 * i - 1.0) * M_PI / (2.0 * filterOrder);
		double si = sin(phi);
		double D = b * b + 2.0 * b * si + 1.0;
		if (filter < 2)   // Compute SOS coefficients
		{
			// High-order HP/LP shelving filter coefficients can be expressed as 2nd-order sections (SOS)
			// i.e. biquads
			coeffs->b0[i - 1] = (g * g * b * b + 2 * g0 * g * b * si + g0 * g0) / D;
			coeffs->b1[i - 1] = (filter == 1) ? -2.0 * (g * g * b * b - g0 * g0) / D : 2.0 * (g * g * b * b - g0 * g0) / D;
			coeffs->b2[i - 1] = (g * g * b * b - 2 * g0 * g * b * si + g0 * g0) / D;
			coeffs->a1[i - 1] = -((filter == 1) ? -2.0 * (b * b - 1.0) / D : 2.0 * (b * b - 1.0) / D);
			coeffs->a2[i - 1] = -((b * b - 2 * b * si + 1.0) / D);
		}
		else if (filter == 2)   // Compute 4th order sections
		{
			// Parameteric EQ filter coefficients (like band pass *band stop) are twice the
			// specified filter order. This is normal and by design. Unlike bandpass *bandstop
			// though, the realization via the Bilinear Transform (BLT) renders 4th order sections.
			// So rather than split 4th order sections into 2nd order sections (biquads),
			// with fancy polynomial root factoring, we use them as is.
			// There are no stability issues for sections this size.
			// The negation conforms the Direct-Form II Transposed discrete-time
			// filter (DF2T) coefficients to the expectations of the process function.
			coeffs->b0[i - 1] = (g * g * b * b + g0 * g0 + 2 * g * g0 * si * b) / D;
			coeffs->b1[i - 1] = -4 * c0 * (g0 * g0 + g * g0 * si * b) / D;
			coeffs->b2[i - 1] = 2 * ((1 + 2 * c0 * c0) * g0 * g0 - g * g * b * b) / D;
			coeffs->b3[i - 1] = -4 * c0 * (g0 * g0 - g * g0 * si * b) / D;
			coeffs->b4[i - 1] = (g * g * b * b + g0 * g0 - 2 * g * g0 * si * b) / D;
			coeffs->a1[i - 1] = -(-4 * c0 * (1 + si * b) / D);
			coeffs->a2[i - 1] = -(2 * (1 + 2 * c0 * c0 - b * b) / D);
			coeffs->a3[i - 1] = -(-4 * c0 * (1 - si * b) / D);
			coeffs->a4[i - 1] = -((b * b - 2 * si * b + 1) / D);
		}
	}
	if (!stateVar->_yn)
	{
		stateVar->_yn = (double*)malloc(coeffs->numFilters * sizeof(double));
		stateVar->_yn1 = (double*)malloc(coeffs->numFilters * sizeof(double));
		stateVar->_yn2 = (double*)malloc(coeffs->numFilters * sizeof(double));
		if (coeffs->filterType == 2)
		{
			stateVar->_yn3 = (double*)malloc(coeffs->numFilters * sizeof(double));
			stateVar->_yn4 = (double*)malloc(coeffs->numFilters * sizeof(double));
			if (!(stateVar->_yn3 || stateVar->_yn4))
				return -1;
		}
		else
		{
			stateVar->_yn3 = 0;
			stateVar->_yn4 = 0;
		}
		shelfReset(coeffs, stateVar);
	}
	return 1;
}
void shelfDeallocateCoeffs(IIRCoefficient *coeffs)
{
	if (coeffs->b0)
	{
		free(coeffs->b0);
		coeffs->b0 = 0;
	}
	if (coeffs->b1)
	{
		free(coeffs->b1);
		coeffs->b1 = 0;
	}
	if (coeffs->b2)
	{
		free(coeffs->b2);
		coeffs->b2 = 0;
	}
	if (coeffs->b3)
	{
		free(coeffs->b3);
		coeffs->b3 = 0;
	}
	if (coeffs->b4)
	{
		free(coeffs->b4);
		coeffs->b4 = 0;
	}
	if (coeffs->a1)
	{
		free(coeffs->a1);
		coeffs->a1 = 0;
	}
	if (coeffs->a2)
	{
		free(coeffs->a2);
		coeffs->a2 = 0;
	}
	if (coeffs->a3)
	{
		free(coeffs->a3);
		coeffs->a3 = 0;
	}
	if (coeffs->a4)
	{
		free(coeffs->a4);
		coeffs->a4 = 0;
	}
}
void shelfDeallocateState(BiquadState *stateVar)
{
	if (stateVar->_yn)
	{
		free(stateVar->_yn);
		stateVar->_yn = 0;
	}
	if (stateVar->_yn1)
	{
		free(stateVar->_yn1);
		stateVar->_yn1 = 0;
	}
	if (stateVar->_yn2)
	{
		free(stateVar->_yn2);
		stateVar->_yn2 = 0;
	}
	if (stateVar->_yn3)
	{
		free(stateVar->_yn3);
		stateVar->_yn3 = 0;
	}
	if (stateVar->_yn4)
	{
		free(stateVar->_yn4);
		stateVar->_yn4 = 0;
	}
}
myFloat shelfProcessBiquad(IIRCoefficient *coeffs, BiquadState *stateVar, const myFloat input)
{
	double *yn = &stateVar->_yn[0];
	double *yn1 = &stateVar->_yn1[0];
	double *yn2 = &stateVar->_yn2[0];
	double xn = input;
	yn[0] = coeffs->b0[0] * xn + coeffs->b1[0] * stateVar->_xn1 + coeffs->b2[0] * stateVar->_xn2
		+ coeffs->a1[0] * yn1[0] + coeffs->a2[0] * yn2[0];
	for (int i = 1; i < coeffs->numFilters; i++)
	{
		yn[i] = coeffs->b0[i] * yn[i - 1] + coeffs->b1[i] * yn1[i - 1] + coeffs->b2[i] * yn2[i - 1]
			+ coeffs->a1[i] * yn1[i] + coeffs->a2[i] * yn2[i];
	}
	// Shift delay line elements.
	for (int i = 0; i < coeffs->numFilters; i++)
	{
		yn2[i] = yn1[i];
		yn1[i] = yn[i];
	}
	stateVar->_xn2 = stateVar->_xn1;
	stateVar->_xn1 = xn;
	return yn[coeffs->numFilters - 1];
}
myFloat shelfProcessFourthOrderSections(IIRCoefficient *coeffs, BiquadState *stateVar, const myFloat input)
{
	double *yn = &stateVar->_yn[0];
	double *yn1 = &stateVar->_yn1[0];
	double *yn2 = &stateVar->_yn2[0];
	double *yn3 = &stateVar->_yn3[0];
	double *yn4 = &stateVar->_yn4[0];
	double xn = input;
	yn[0] = coeffs->b0[0] * xn
		+ coeffs->b1[0] * stateVar->_xn1
		+ coeffs->b2[0] * stateVar->_xn2
		+ coeffs->b3[0] * stateVar->xn3
		+ coeffs->b4[0] * stateVar->xn4
		+ coeffs->a1[0] * yn1[0]
		+ coeffs->a2[0] * yn2[0]
		+ coeffs->a3[0] * yn3[0]
		+ coeffs->a4[0] * yn4[0];
	for (int i = 1; i < coeffs->numFilters; i++)
	{
		yn[i] = coeffs->b0[i] * yn[i - 1]
			+ coeffs->b1[i] * yn1[i - 1]
			+ coeffs->b2[i] * yn2[i - 1]
			+ coeffs->b3[i] * yn3[i - 1]
			+ coeffs->b4[i] * yn4[i - 1]
			+ coeffs->a1[i] * yn1[i]
			+ coeffs->a2[i] * yn2[i]
			+ coeffs->a3[i] * yn3[i]
			+ coeffs->a4[i] * yn4[i];
	}
	for (int i = 0; i < coeffs->numFilters; i++)
	{
		yn4[i] = yn3[i];
		yn3[i] = yn2[i];
		yn2[i] = yn1[i];
		yn1[i] = yn[i];
	}
	stateVar->xn4 = stateVar->xn3;
	stateVar->xn3 = stateVar->_xn2;
	stateVar->_xn2 = stateVar->_xn1;
	stateVar->_xn1 = xn;
	return yn[coeffs->numFilters - 1];
}