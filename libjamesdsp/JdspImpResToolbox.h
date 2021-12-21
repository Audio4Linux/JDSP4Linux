#ifndef JDSPIMPRESTOOLBOX_H
#define JDSPIMPRESTOOLBOX_H

extern float* ReadImpulseResponseToFloat(const char* mIRFileName, int targetSampleRate, int* jImpInfo, int convMode, int* javaAdvSetPtr);
extern int ComputeEqResponse(const double* jfreq, double* jgain, int interpolationMode, int queryPts, double* dispFreq, float* response);

#endif // JDSPIMPRESTOOLBOX_H
