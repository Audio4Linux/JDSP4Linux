/*
	Windowed Sinc FIR Generator
	Usage:
		Lowpass:	JfirLP(h, N, WINDOW, fc)
		Highpass:	JfirHP(h, N, WINDOW, fc)
		Bandpass:	JfirBP(h, N, WINDOW, fc1, fc2)
		Bandstop:	JfirBS(h, N, WINDOW, fc1, fc2)

	where:
		h (double[N]):	filter coefficients will be written to this array
		N (int):		number of taps
		WINDOW (int):	Window (W_BLACKMAN, W_HANNING, or W_HAMMING)
		fc (double):	cutoff (0 < fc < 0.5, fc = f/fs)
						--> for fs=48kHz and cutoff f=12kHz, fc = 12k/48k => 0.25
		fc1 (double):	low cutoff (0 < fc < 0.5, fc = f/fs)
		fc2 (double):	high cutoff (0 < fc < 0.5, fc = f/fs)
*/
#ifndef Jfir_H
#define Jfir_H

#include <math.h>

// Function prototypes
void JfirLP(double h[], int &N, const int &WINDOW, const double &fc, double &Beta);
void JfirHP(double h[], int &N, const int &WINDOW, const double &fc, double &Beta);
void JfirBS(double h[], int &N, const int &WINDOW, const double &fc1, const double &fc2, double &Beta);
void JfirBP(double h[], int &N, const int &WINDOW, const double &fc1, const double &fc2, double &Beta);
void genSincFx(double sincfx[], const int &N, const double &fc);
void wNone(double w[], const int &N);
void wBlackman(double w[], const int &N);
void wHanning(double w[], const int &N);
void wHamming(double w[], const int &N);
void wKaiser(double w[], const int &N, double &Beta);
void wSinc(double w[], int &N, double &Beta);
double BesselCal(double x);
double sincCal(double x);

// Window type contstants
const int W_NONE = 1;
const int W_BLACKMAN = 2;
const int W_HANNING = 3;
const int W_HAMMING = 4;
const int W_KAISER = 5;
const int W_SINC = 6;

// Generate lowpass filter
// 
// This is done by generating a sinc function and then windowing it
void JfirLP(double h[],		// h[] will be written with the filter coefficients
			 int &N,		// size of the filter (number of taps)
			 const int &WINDOW,	// window function (W_BLACKMAN, W_HANNING, etc.)
			 const double &fc,	// cutoff frequency
			 double &Beta) // atteuate
{
	int i;
	double *w = new double[N];		// window function
    double *sincfx = new double[N];	// sinc function
    
	// 1. Generate Sinc function
	genSincFx(sincfx, N, fc);
    
	// 2. Generate Window function
	switch (WINDOW) {
		case W_NONE:
			wNone(w, N);
			break;
		case W_BLACKMAN:	// W_BLACKMAN
			wBlackman(w, N);
			break;
		case W_HANNING:		// W_HANNING
			wHanning(w, N);
			break;
		case W_HAMMING:		// W_HAMMING
			wHamming(w, N);
			break;
		case W_KAISER:		// W_KAISER
			wKaiser(w, N, Beta);
			break;
		case W_SINC:		// W_SINC
			wSinc(w, N, Beta);
			break;
		default:
			break;
	}

	// 3. Make lowpass filter
	for (i = 0; i < N; i++) {
		h[i] = sincfx[i] * w[i];
	}

	// Delete dynamic storage
	delete []w;
	delete []sincfx;

	return;
}

// Generate highpass filter
//
// This is done by generating a lowpass filter and then spectrally inverting it
void JfirHP(double h[],		// h[] will be written with the filter coefficients
			 int &N,		// size of the filter
			 const int &WINDOW,	// window function (W_BLACKMAN, W_HANNING, etc.)
			 const double &fc,	// cutoff frequency
			 double &Beta) // atteuate
{
	int i;
	if (N % 2 == 0)
	{
		N = N + 1;
	}
	else
	{
	}
	// 1. Generate lowpass filter
	JfirLP(h, N, WINDOW, fc, Beta);

	// 2. Spectrally invert (negate all samples and add 1 to center sample) lowpass filter
	// = delta[n-((N-1)/2)] - h[n], in the time domain
	for (i = 0; i < N; i++) {
		h[i] *= -1.0;	// = 0 - h[i]
	}
	h[(N-1)/2] += 1.0;	// = 1 - h[(N-1)/2]

	return;
}

// Generate bandstop filter
//
// This is done by generating a lowpass and highpass filter and adding them
void JfirBS(double h[],		// h[] will be written with the filter taps
			 int &N,		// size of the filter
			 const int &WINDOW,	// window function (W_BLACKMAN, W_HANNING, etc.)
			 const double &fc1,	// low cutoff frequency
			 const double &fc2,	// cutoff frequency
			 double &Beta) // atteuate
{
	int i;
	if (N % 2 == 0)
	{
		N = N + 1;
	}
	else
	{
	}
	double *h1 = new double[N];
	double *h2 = new double[N];

	// 1. Generate lowpass filter at first (low) cutoff frequency
	JfirLP(h1, N, WINDOW, fc1, Beta);

	// 2. Generate highpass filter at second (high) cutoff frequency
	JfirHP(h2, N, WINDOW, fc2, Beta);

	// 3. Add the 2 filters together
	for (i = 0; i < N; i++) {
		h[i] = h1[i] + h2[i];
	}

	// Delete dynamic memory
	delete []h1;
	delete []h2;

	return;
}

// Generate bandpass filter
//
// This is done by generating a bandstop filter and spectrally inverting it
void JfirBP(double h[],		// h[] will be written with the filter taps
			 int &N,		// size of the filter
			 const int &WINDOW,	// window function (W_BLACKMAN, W_HANNING, etc.)
			 const double &fc1,	// low cutoff frequency
			 const double &fc2,	// cutoff frequency
			 double &Beta) // atteuate
{
	int i;

	// 1. Generate bandstop filter
	JfirBS(h, N, WINDOW, fc1, fc2, Beta);

	// 2. Spectrally invert bandstop (negate all, and add 1 to center sample)
	for (i = 0; i < N; i++) {
		h[i] *= -1.0;	// = 0 - h[i]
	}
	h[(N-1)/2] += 1.0;	// = 1 - h[(N-1)/2]

	return;
}

// Generate sinc function - used for making lowpass filter
void genSincFx(double sincfx[],		// sincfx[] will be written with the sinc function
			 const int &N,		// size (number of taps)
			 const double &fc)	// cutoff frequency
{
	int i;
	const double M = N-1;
	double n;

	// Constants
	const double PI = 3.14159265358979323846;

	// Generate sinc delayed by (N-1)/2
	for (i = 0; i < N; i++) {
		if (i == M/2.0) {
			sincfx[i] = 2.0 * fc;
		}
		else {
			n = (double)i - M/2.0;
			sincfx[i] = sin(2.0*PI*fc*n) / (PI*n);
		}
	}        

	return;
}

// Generate window function (None)
void wNone(double w[],		// w[] will be written with the Blackman window
           const int &N)	// size of the window
{
	int i;
	for (i = 0; i < N; i++) {
		w[i] = 1;
	}

	return;
}

// Generate window function (Blackman)
void wBlackman(double w[],		// w[] will be written with the Blackman window
			   const int &N)	// size of the window
{
	int i;
	const double M = N-1;
	const double PI = 3.14159265358979323846;

	for (i = 0; i < N; i++) {
		w[i] = 0.42 - (0.5 * cos(2.0*PI*(double)i/M)) + (0.08*cos(4.0*PI*(double)i/M));
	}

	return;
}

// Generate window function (Hanning)
void wHanning(double w[],		// w[] will be written with the Hanning window
			  const int &N)		// size of the window
{
	int i;
	const double M = N-1;
	const double PI = 3.14159265358979323846;

	for (i = 0; i < N; i++) {
		w[i] = 0.5 * (1.0 - cos(2.0*PI*(double)i/M));
	}

	return;
}

// Generate window function (Hamming)
void wHamming(double w[],		// w[] will be written with the Hamming window
			  const int &N)		// size of the window
{
	int i;
	const double M = N-1;
	const double PI = 3.14159265358979323846;

	for (i = 0; i < N; i++) {
		w[i] = 0.54 - (0.46*cos(2.0*PI*(double)i/M));
	}

	return;
}
void wKaiser(double w[], const int &N, double &Beta)
{
	if (Beta < 0.0)Beta = 0.0;
	if (Beta > 10.0)Beta = 10.0;
	int i, M, dM, TopWidth;
	double Arg;
	TopWidth = (int)(0 * (double)N);
	if (TopWidth % 2 != 0)TopWidth++;
	if (TopWidth > N)TopWidth = N;
	M = N - TopWidth;
	dM = M + 1;

	for (i = 0; i < M; i++) {
		Arg = Beta * sqrt(1.0 - pow(((double)(2 * i + 2) - dM) / dM, 2.0));
		w[i] = BesselCal(Arg) / BesselCal(Beta);
}

	return;
}
void wSinc(double w[], int &N, double &Beta)
{
	const double PI = 3.14159265358979323846;
	if (Beta < 0.0)Beta = 0.0;
	if (Beta > 10.0)Beta = 10.0;
	int i, M, dM, TopWidth;
	TopWidth = (int)(0 * (double)N);
	if (TopWidth % 2 != 0)TopWidth++;
	if (TopWidth > N)TopWidth = N;
	M = N - TopWidth;
	dM = M + 1;

	for (i = 0; i < M; i++) w[i] = sincCal((double)(2 * i + 1 - M) / dM * PI);
	for (i = 0; i < M; i++) w[i] = pow(w[i], Beta);
	return;
}
double BesselCal(double x)
{
	double Sum = 0.0, XtoIpower;
	int i, j, Factorial;
	for (i = 1; i<10; i++)
	{
		XtoIpower = pow(x / 2.0, (double)i);
		Factorial = 1;
		for (j = 1; j <= i; j++)Factorial *= j;
		Sum += pow(XtoIpower / (double)Factorial, 2.0);
	}
	return(1.0 + Sum);
}
// This gets used with the Sinc window.
double sincCal(double x)
{
	if (x > -1.0E-5 && x < 1.0E-5)return(1.0);
	return(sin(x) / x);
}
#endif