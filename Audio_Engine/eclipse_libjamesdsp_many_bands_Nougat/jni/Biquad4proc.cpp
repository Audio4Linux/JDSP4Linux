#include "Biquad4proc.h"
#include <math.h>

static int64_t toFixedPoint(double in) {
    return int64_t(0.5 + in * (int64_t(1) << 32));
}

Biquad4proc::Biquad4proc()
{
    reset();
    setCoefficients(1, 0, 0, 1, 0, 0);
}

Biquad4proc::~Biquad4proc()
{
}

void Biquad4proc::setCoefficients(double a0, double a1, double a2, double b0, double b1, double b2)
{
    int64_t A1 = -toFixedPoint(a1/a0);
    int64_t A2 = -toFixedPoint(a2/a0);
    int64_t B0 = toFixedPoint(b0/a0);
    int64_t B1 = toFixedPoint(b1/a0);
    int64_t B2 = toFixedPoint(b2/a0);
	mA1 = A1;
    mA2 = A2;
    mB0 = B0;
    mB1 = B1;
    mB2 = B2;
}

void Biquad4proc::reset()
{
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

void Biquad4proc::setPeaking(double center_frequency, double sampling_frequency, double gainDb, double slope)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double A = pow(10, gainDb/40);
    double alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/slope - 1) + 2 );

    double b0 =   1 + alpha*A;
    double b1 =  -2*cos(w0);
    double b2 =   1 - alpha*A;
    double a0 =   1 + alpha/A;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha/A;

    setCoefficients(a0, a1, a2, b0, b1, b2);
}

void Biquad4proc::setHighShelf(double center_frequency, double sampling_frequency, double gainDb, double slope)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double A = pow(10, gainDb/40);
    double alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/slope - 1) + 2 );

    double b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha );
    double b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   );
    double b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha );
    double a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha  ;
    double a1 =    2*( (A-1) - (A+1)*cos(w0)                   );
    double a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha  ;

    setCoefficients(a0, a1, a2, b0, b1, b2);
}

void Biquad4proc::setBandPass(double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =   sin(w0)/2;
    double b1 =   0;
    double b2 =  -sin(w0)/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(a0, a1, a2, b0, b1, b2);
}

void Biquad4proc::setHighPass(double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =  (1 + cos(w0))/2;
    double b1 = -(1 + cos(w0));
    double b2 =  (1 + cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(a0, a1, a2, b0, b1, b2);
}

void Biquad4proc::setLowPass(double center_frequency, double sampling_frequency, double resonance)
{
    double w0 = 2 * M_PI * center_frequency / sampling_frequency;
    double alpha = sin(w0) / (2*resonance);

    double b0 =  (1 - cos(w0))/2;
    double b1 =   1 - cos(w0);
    double b2 =  (1 - cos(w0))/2;
    double a0 =   1 + alpha;
    double a1 =  -2*cos(w0);
    double a2 =   1 - alpha;

    setCoefficients(a0, a1, a2, b0, b1, b2);
}

void Biquad4proc::setSOS(double a0, double a1, double a2, double b0, double b1, double b2)
{
	setCoefficients(a0, a1, a2, b0, b1, b2);
}

int32_t Biquad4proc::process(int32_t x0)
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

    return y0;
}
