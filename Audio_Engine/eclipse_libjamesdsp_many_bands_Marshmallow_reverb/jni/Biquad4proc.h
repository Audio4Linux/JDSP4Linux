#pragma once

#include <stdint.h>

class Biquad4proc {
    protected:
    int32_t mX1, mX2;
    int32_t mY1, mY2;
    int64_t mB0, mB1, mB2, mA1, mA2;

    void setCoefficients(double a0, double a1, double a2, double b0, double b1, double b2);

    public:
    Biquad4proc();
    virtual ~Biquad4proc();
    void setPeaking(double cf, double sf, double gaindB, double slope);
    void setHighShelf(double cf, double sf, double gaindB, double slope);
    void setBandPass(double cf, double sf, double resonance);
    void setHighPass(double cf, double sf, double resonance);
    void setLowPass(double cf, double sf, double resonance);
    void setSOS(double a0, double a1, double a2, double b0, double b1, double b2);
    int32_t process(int32_t in);
    void reset();
};
