#include <stdlib.h>
float* SplineAllocate(size_t n);
int spline(const float *px, const float *py, size_t n, float *py2, float *pu);
int splint(const float *pxa, const float *pya, const float *py2a, size_t n, const float* px, float *py, size_t nx, int blookup);
void linspace(float *x, int n, float a, float b);