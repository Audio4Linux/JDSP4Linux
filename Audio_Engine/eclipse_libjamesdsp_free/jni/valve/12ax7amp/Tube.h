#ifndef __TUBE_H__
#define __TUBE_H__
#include "wdfcircuits_triode.h"
typedef struct str_tubeFilter
{
    Triode v;
    TubeStageCircuit ckt;
    Real e;
    Real ci;
    Real ck;
    Real co;
    Real ro;
    Real rp;
    Real rg;
    Real ri;
	Real rk;
    float overdrived1Gain;
    float overdrived2Gain;
} tubeFilter;
int InitTube(tubeFilter *tubefilter, double *circuitparameters, float samplerate, float tubedrive, int warmupDuration, int insane);
void processTube(tubeFilter *tubefilter, float* inputs, float* outputs, unsigned frames);
#endif
