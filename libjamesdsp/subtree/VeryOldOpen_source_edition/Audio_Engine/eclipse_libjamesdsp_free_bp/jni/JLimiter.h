typedef struct JLimiter
{
	double threshold;
	double relCoef;
	double envOverThreshold;
} JLimiter;
void JLimiterProcess(JLimiter *limiter, double *in1, double *in2);
void JLimiterProcessFloat(JLimiter *limiter, float *in1, float *in2);
void JLimiterSetCoefficients(JLimiter *limiter, double thresholddB, double msRelease, double fs);
void JLimiterInit(JLimiter *limiter);