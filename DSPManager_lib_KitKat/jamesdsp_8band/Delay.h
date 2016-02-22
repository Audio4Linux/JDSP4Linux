#pragma once

#include <stdint.h>

class Delay {
    int32_t* mState;
    int32_t mIndex;
    int32_t mLength;

    public:
    Delay();
    ~Delay();
    void setParameters(float rate, float time);
    int32_t process(int32_t x0);
};
