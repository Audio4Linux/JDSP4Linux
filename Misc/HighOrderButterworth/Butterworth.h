#define myFloat double
typedef struct strBiquadState
{
	double _xn1, _xn2;
	double xn3, xn4;
	double *_yn, *_yn1, *_yn2;
	double *_yn3, *_yn4;
} BiquadState;
typedef struct strIIRCoefficient
{
	int numFilters, filterType, interpolationStep;
	double *b0, *b1, *b2, *a1, *a2;
	double *a3, *a4, *b3, *b4;
	myFloat(*process)(struct strIIRCoefficient*, BiquadState*, myFloat);
} IIRCoefficient;
// State variable reset
void shelfReset(IIRCoefficient *coeffs, BiquadState *stateVar);
// Generate shelf coefficients, allocate and save it to coeffs struct, which is provided by user, allocation on needed, changing order will lead to re-allocation
int shelfCoefficientsGen(IIRCoefficient *coeffs, BiquadState *stateVar, unsigned int filter, double fs, double f1, double f2, unsigned int filterOrder, double overallGain);
// Biquad sections and IIR coefficient clean up
void shelfDeallocateCoeffs(IIRCoefficient *coeffs);
void shelfDeallocateState(BiquadState *stateVar);
// Process the biquad filter chain on a input sample, write to output buffer
myFloat shelfProcessBiquad(IIRCoefficient *coeffs, BiquadState *stateVar, const myFloat input);
myFloat shelfProcessFourthOrderSections(IIRCoefficient *coeffs, BiquadState *stateVar, const myFloat input);