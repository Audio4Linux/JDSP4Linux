#include "Biquad.h"
#define M_PI 3.14159265358979323846
static int64_t toFixedPoint(double in) {
    return int64_t(0.5 + in * (int64_t(1) << 32));
}
static double fast_sin(double x) {
	int k;
	double y;
	double z;

	z  = x;
	z *= 0.3183098861837907;
	z += 6755399441055744.0;
	k  = *((int *) &z);
	z  = k;
	z *= 3.1415926535897932;
	x -= z;
	y  = x;
	y *= x;
	z  = 0.0073524681968701;
	z *= y;
	z -= 0.1652891139701474;
	z *= y;
	z += 0.9996919862959676;
	x *= z;
	k &= 1;
	k += k;
	z  = k;
	z *= x;
	x -= z;

	return x;
}
static double fast_cos(double x)
{
	return (fast_sin(x + 1.570796326794897));
}
static double fastPow(double a, double b)
{
    union
{
    double d;
    int x[2];
}
u = {a};
u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
u.x[0] = 0;
return u.d;
}
static double fast_sqrt(double n)
{
    double a = n;
    double x =1;
    for(int i = 0; i < n; i++)
        {
            x = 0.5 * (x + a / x);
        }
return x;
}


Biquad::Biquad()
{
    reset();
    setCoefficients(0, 1, 0, 0, 1, 0, 0);
}

Biquad::~Biquad()
{
}

void Biquad::setCoefficients(int32_t steps, double a0, double a1, double a2, double b0, double b1, double b2)
{
    int64_t A1 = -toFixedPoint(a1/a0);
    int64_t A2 = -toFixedPoint(a2/a0);
    int64_t B0 = toFixedPoint(b0/a0);
    int64_t B1 = toFixedPoint(b1/a0);
    int64_t B2 = toFixedPoint(b2/a0);

    if (steps == 0) {
        mA1 = A1;
        mA2 = A2;
        mB0 = B0;
        mB1 = B1;
        mB2 = B2;
        mInterpolationSteps = 0;
    } else {
        mA1dif = (A1 - mA1) / steps;
        mA2dif = (A2 - mA2) / steps;
        mB0dif = (B0 - mB0) / steps;
        mB1dif = (B1 - mB1) / steps;
        mB2dif = (B2 - mB2) / steps;
        mInterpolationSteps = steps;
    }
}

void Biquad::reset()
{
    mInterpolationSteps = 0;
    mA1 = 0;
    mA2 = 0;
    mB0 = 0;
    mB1 = 0;
    mB2 = 0;
    mX1 = 0;
    mX2 = 0;
    mY1 = 0;
    mY2 = 0;
}

void Biquad::setHighShelf(int32_t steps, double center_frequency, double sampling_frequency, double gainDb, double slope, double overallGainDb)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double A = fastPow(10, gainDb/40);
    double alpha = fast_sin(w0)/2 * fast_sqrt( (A + 1/A)*(1/slope - 1) + 2 );

    double b0 =    A*( (A+1) + (A-1)*fast_cos(w0) + 2*fast_sqrt(A)*alpha );
    double b1 = -2*A*( (A-1) + (A+1)*fast_cos(w0)                   );
    double b2 =    A*( (A+1) + (A-1)*fast_cos(w0) - 2*fast_sqrt(A)*alpha );
    double a0 =        (A+1) - (A-1)*fast_cos(w0) + 2*fast_sqrt(A)*alpha  ;
    double a1 =    2*( (A-1) - (A+1)*fast_cos(w0)                   );
    double a2 =        (A+1) - (A-1)*fast_cos(w0) - 2*fast_sqrt(A)*alpha  ;

    double overallGain = fastPow(10, overallGainDb / 20);
    b0 *= overallGain;
    b1 *= overallGain;
    b2 *= overallGain;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setBandPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = fast_sin(w0) / (2*resonance);

    double b0 =   fast_sin(w0)/2;
    double b1 =   0;
    double b2 =  -fast_sin(w0)/2;
    double a0 =   1 + alpha;
    double a1 =  -2*fast_cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setHighPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = fast_sin(w0) / (2*resonance);

    double b0 =  (1 + fast_cos(w0))/2;
    double b1 = -(1 + fast_cos(w0));
    double b2 =  (1 + fast_cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*fast_cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

void Biquad::setLowPass(int32_t steps, double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = fast_sin(w0) / (2*resonance);

    double b0 =  (1 - fast_cos(w0))/2;
    double b1 =   1 - fast_cos(w0);
    double b2 =  (1 - fast_cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*fast_cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(steps, a0, a1, a2, b0, b1, b2);
}

int32_t Biquad::process(int32_t x0)
{
    int64_t y0 = mB0 * x0
        + mB1 * mX1
        + mB2 * mX2
        + mA1 * mY1
        + mA2 * mY2;
    y0 >>= 32;

    mY2 = mY1;
    mY1 = y0;

    mX2 = mX1;
    mX1 = x0;

    /* Interpolate biquad parameters */
    if (mInterpolationSteps != 0) {
        mInterpolationSteps --;
        mB0 += mB0dif;
        mB1 += mB1dif;
        mB2 += mB2dif;
        mA1 += mA1dif;
        mA2 += mA2dif;
    }

    return y0;
}
