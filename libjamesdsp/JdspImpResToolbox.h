#ifndef JDSPIMPRESTOOLBOX_H
#define JDSPIMPRESTOOLBOX_H

extern float* ReadImpulseResponseToFloat(const char* mIRFileName, int targetSampleRate, int* jImpInfo, int convMode, int* javaAdvSetPtr);
extern int ComputeEqResponse(const double* jfreq, double* jgain, int interpolationMode, int queryPts, double* dispFreq, float* response);
extern int ComputeCompResponse(int n, const double* jfreq, const double* jgain, int queryPts, const double* dispFreq, float* response);
extern void ComputeIIREqualizerCplx(int srate, int order, const double* freq, double* gains, int nPts, const double* dispFreq, double* cplxRe, double* cplxIm);
extern void ComputeIIREqualizerResponse(int nPts, const double* cplxRe, const double* cplxIm, float* response);
#endif // JDSPIMPRESTOOLBOX_H
