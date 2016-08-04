#pragma once

#include <stdint.h>

class FIR16 {
    int64_t mCoeff[16];
    int32_t mState[16];
    int32_t mIndex;

    public:
    FIR16();
    ~FIR16();
    void setParameters(float coeff[16]);
    int32_t process(int32_t x0);
};
