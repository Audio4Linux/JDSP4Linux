#include <math.h>
#include "JLimiter.h"
void JLimiterProcessFloat(JLimiter *limiter, float *in1, float *in2)
{
	float rect1 = fabsf(*in1);
	float rect2 = fabsf(*in2);
	double maxLR = (double)fmaxf(rect1, rect2);
	if (maxLR < limiter->threshold)
		maxLR = limiter->threshold;
	if (maxLR > limiter->envOverThreshold)
		limiter->envOverThreshold = maxLR;
	else
		limiter->envOverThreshold = maxLR + limiter->relCoef * (limiter->envOverThreshold - maxLR);
	double gR = limiter->threshold / limiter->envOverThreshold;
	*in1 = *in1 * (float)gR;
	*in2 = *in2 * (float)gR;
}
void JLimiterProcess(JLimiter *limiter, double *in1, double *in2)
{
	double rect1 = fabs(*in1);
	double rect2 = fabs(*in2);
	double maxLR = fmax(rect1, rect2);
	if (maxLR < limiter->threshold)
		maxLR = limiter->threshold;
	if (maxLR > limiter->envOverThreshold)
		limiter->envOverThreshold = maxLR;
	else
		limiter->envOverThreshold = maxLR + limiter->relCoef * (limiter->envOverThreshold - maxLR);
	double gR = limiter->threshold / limiter->envOverThreshold;
	*in1 *= gR;
	*in2 *= gR;
}
void JLimiterSetCoefficients(JLimiter *limiter, double thresholddB, double msRelease, double fs)
{
	if (msRelease < 1.5)
		msRelease = 1.5;
	limiter->relCoef = exp(-1000.0 / (msRelease * fs));
	limiter->threshold = pow(10.0, thresholddB / 20.0);
}
void JLimiterInit(JLimiter *limiter)
{
	limiter->envOverThreshold = 0.0;
}