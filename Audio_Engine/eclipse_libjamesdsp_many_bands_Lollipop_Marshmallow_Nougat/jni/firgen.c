#include <math.h>
#include <stdlib.h>
#include "firgen.h"
float BesselCal(float x)
{
	float Sum = 0.0, XtoIpower;
	int i, j, Factorial;
	for (i = 1; i<10; i++)
	{
		XtoIpower = powf(x / 2.0, (float)i);
		Factorial = 1;
		for (j = 1; j <= i; j++)Factorial *= j;
		Sum += powf(XtoIpower / (float)Factorial, 2.0);
	}
	return(1.0 + Sum);
}
void wKaiser(float w[], const int N, float beta)
{
	if (beta < 0.0f)beta = 0.0f;
	if (beta > 10.0f)beta = 10.0f;
	int i, M, dM, TopWidth;
	float Arg;
	TopWidth = (int)(0 * (float)N);
	if (TopWidth % 2 != 0)TopWidth++;
	if (TopWidth > N)TopWidth = N;
	M = N - TopWidth;
	dM = M + 1;
	for (i = 0; i < M; i++)
	{
		Arg = beta * sqrt(1.0 - powf(((float)(2.0f * (float)i + 2.0f) - dM) / dM, 2.0f));
		w[i] = BesselCal(Arg) / BesselCal(beta);
	}
	return;
}
void genSincFx(float *sincfx, int *N, const float fc)
{
	int i;
	const float M = *N - 1;
	float n;
	for (i = 0; i < *N; i++) {
		if (i == M / 2.0f) {
			sincfx[i] = 2.0f * fc;
		}
		else {
			n = (float)i - M / 2.0f;
			sincfx[i] = sinf(2.0f*3.14159265f*fc*n) / (3.14159265f*n);
		}
	}
}
void JfirLP(float *h, int *N, const int wnd, const float fc, float beta, float gain)
{
	int i;
	float *w = (float*)malloc(*N * sizeof(float));
	float *sincfx = (float*)malloc(*N * sizeof(float));
	genSincFx(sincfx, N, fc);
	if (wnd)
	{
		wKaiser(w, *N, beta);
		for (i = 0; i < *N; i++)
			h[i] = sincfx[i] * w[i] * gain;
	}
	else
	{
		for (i = 0; i < *N; i++)
			h[i] = sincfx[i] * gain;
	}
	free(w);
	free(sincfx);
}
void JfirHP(float *h, int *N, const int wnd, const float fc, float beta, float gain)
{
	int i;
	if (*N % 2 == 0)
		*N -= 1;
	JfirLP(h, N, wnd, fc, beta, gain);
	for (i = 0; i < *N; i++) {
		h[i] *= -1.0;
	}
	h[(*N - 1) / 2] += 1.0;
	return;
}
void JfirBS(float *h, int *N, const int wnd, const float fc1, const float fc2, float beta, float gain)
{
	int i;
	if (*N % 2 == 0)
		*N -= 1;
	float *h1 = (float*)malloc(*N * sizeof(float));
	float *h2 = (float*)malloc(*N * sizeof(float));
	JfirLP(h1, N, wnd, fc1, beta, gain);
	JfirHP(h2, N, wnd, fc2, beta, gain);
	for (i = 0; i < *N; i++) {
		h[i] = h1[i] + h2[i];
	}
	free(h1);
	free(h2);
	return;
}
void JfirBP(float *h, int *N, const int wnd, const float fc1, const float fc2, float beta, float gain)
{
	int i;
	JfirBS(h, N, wnd, fc1, fc2, beta, gain);
	for (i = 0; i < *N; i++) {
		h[i] *= -1.0f;
	}
	h[(*N - 1) / 2] += 1.0f;
	return;
}
