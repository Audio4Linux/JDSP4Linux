#pragma once

#include <stdint.h>

class FIR512 {
    int64_t mCoeff[512];
    int32_t mState[512];
    int32_t mIndex;

    public:
    FIR512();
    ~FIR512();
    void setParameters(double coeff[512]);
    int32_t process(int32_t x0);
};
