#ifndef JDSPIMPRESTOOLBOX_H
#define JDSPIMPRESTOOLBOX_H

/*
 * This file contains modified code that was originally written by github.com/james34602
 * Source: https://github.com/james34602/JamesDSPManager/blob/master/Main/DSPManager/jni/main/JdspImpResToolbox.c
 * Licensed under GPLv3
 */

#include <stdlib.h>
#include <string.h>

extern "C" {
#include "interpolation.h"
#define NUMPTS 15
ierper pch1, pch2;
__attribute__((constructor)) static void initialize(void)
{
    initIerper(&pch1, NUMPTS + 2);
    initIerper(&pch2, NUMPTS + 2);
}
__attribute__((destructor)) static void destruction(void)
{
    freeIerper(&pch1);
    freeIerper(&pch2);
}

double freq[NUMPTS + 2];
double gain[NUMPTS + 2];
int ComputeEqResponse(double* jfreq, double* jgain, int interpolationMode, int queryPts, double* dispFreq, float* response)
{
    memcpy(freq + 1, jfreq, NUMPTS * sizeof(double));
    memcpy(gain + 1, jgain, NUMPTS * sizeof(double));
    freq[0] = 0.0;
    gain[0] = gain[1];
    freq[NUMPTS + 1] = 24000.0;
    gain[NUMPTS + 1] = gain[NUMPTS];
    ierper *lerpPtr;
    if (!interpolationMode)
    {
        pchip(&pch1, freq, gain, NUMPTS + 2, 1, 1);
        lerpPtr = &pch1;
    }
    else
    {
        makima(&pch2, freq, gain, NUMPTS + 2, 1, 1);
        lerpPtr = &pch2;
    }
    for (int i = 0; i < queryPts; i++)
    {
        response[i] = (float)getValueAt(&lerpPtr->cb, dispFreq[i]);
    }
    return 0;
}
}
#endif // JDSPIMPRESTOOLBOX_H
