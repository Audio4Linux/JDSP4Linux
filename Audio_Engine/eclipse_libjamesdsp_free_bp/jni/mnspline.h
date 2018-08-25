#include <stdlib.h>
double* SplineAllocate(size_t n);
int spline(const double *px, const double *py, size_t n, double *py2, double *pu);
int splint(const double *pxa, const double *pya, const double *py2a, size_t n, const double* px, double *py, size_t nx, int blookup);
void linspace(double *x, int n, double a, double b);