/* mnspline.c
 * Natural cubic spline interpolation
 * 
 * adapted from "3.3 Cubic Spline Interpolation" in 
 * Numerical Recipes in C: The Art of Scientific Computing, 2nd edition.
 *
 * with additions: 
 *  performs the lookup on the array in parallel (with OpenMP)
 *  caches the previous lookup results, with either a linear probe or bisection
 */
#include "mnspline.h"
#include <stdlib.h>

void linspace(double *x, int n, double a, double b)
{
	int i;
	double d = (b - a) / (double)(n - 1);
	for (i = 0; i < n; i++)
		x[i] = a + i * d;
}
/* Returns an index i s.t. pxa[i] <= x < pxa[i+1] */
inline size_t b_search(const double *pxa, double x, size_t idxlow, size_t idxhigh)
{
	size_t ilo = idxlow;
	size_t ihi = idxhigh;
	size_t mid;
	while (ihi > ilo + 1)
	{
		mid = ilo + ((ihi - ilo) / 2);
		if (pxa[mid] > x)
			ihi = mid;
		else
			ilo = mid;
	}
	return ilo;
}

/* Linear probing for an i st. pxa[i] <= x < pxa[i+1] */
inline int lin_search(const double *pxa, double x, size_t idxlow, size_t idxhigh, size_t *index)
{
	int found = 0;
	for (size_t i = idxlow; i < idxhigh; i++)
	{
		if (pxa[i] <= x && pxa[i + 1] > x)
		{
			found = 1;
			*index = i;
			break;
		}
	}
	return found;
}
double* SplineAllocate(size_t n)
{
	double *pu;
	if ((pu = (double*)malloc((n - 1) * sizeof(double))) == NULL)
		return 0;
	return pu;
}
/* spline - calculate the 2nd. derivatives of the interpolating function
 * only needs to be called once - gives input to splint
 * the boundary condition at x1 and xN is zero.
 * Does no input validation (can be done in wrapper)
 *
 * Returns -1 on malloc failure.  
 *
 * px: Ptr to array of function evaluation points, x[1] < x[2] < ... < x[n] 
 * py: Ptr to array of function evaluated at above points
 * n: Size of array px and py
 * py2: Ptr to array, returns the 2nd derivatives of the interpolating function
 * pu: Working buffer from SplineAllocate(), this prevent calling malloc multiple time to improve performance
 */
int spline(const double *px, const double *py, size_t n, double *py2, double *pu)
{
    double qn = 0.0; /* Upper boundary condition set to be 'natural' */
    double un = 0.0;  /* */
    double p;
    double sig;
    py2[0] = 0.0; /* Lower boundary condition set to be 'natural' */
    pu[0] = 0.0;  /* */

    /* The Tridiagonal algorithm */
    for (size_t i = 1; i < n - 1; i++)
    {
        sig     = (px[i] - px[i-1]) / (px[i+1] - px[i-1]);
        p       = sig * py2[i-1] + 2.0;
        py2[i]  = (sig - 1.0) / p;
        pu[i]   = (py[i+1] - py[i]) / (px[i+1] - px[i]) - (py[i] - py[i-1]) / (px[i] - px[i-1]);
        pu[i]   = (6.0 * pu[i] / (px[i+1] - px[i-1]) - sig * pu[i-1]) / p; 
    }
    py2[n-1] = (un - qn * pu[n-2]) / (qn * py2[n-2] + 1.0);
    for (size_t k = n - 1; k-- > 0; )
        py2[k] = py2[k] * py2[k+1] + pu[k];
    return 0;
}

/* splint - perform the interpolation
 * returns 0 - currently does no error checking (can be done in wrapper)
 *
 * pxa: Same input as to spline - px
 * pya: Same input as to spline - py
 * py2a: The output from spline - py2
 * n: Same input as to spline - n
 * px: Ptr to array of function evaluation points to be interpolated
 * py: Ptr to array, returns the interpolated function values evaluated at px
 * nx: Size of array px and py
 * blookup: 0=does linear probe on interpolation table,
 *          1=does binary search on interpolation table
 */
int splint(const double *pxa, const double *pya, const double *py2a, size_t n, const double* px, double *py, size_t nx, int blookup)
{
    size_t klo;
    size_t khi;
    size_t prev_idx = 0;

    double h;
    double b;
    double a;
    double x;

//    #pragma omp parallel for firstprivate(prev_idx) private(klo, khi, h, b, a, x)
    for (size_t i = 0; i < nx; i++)
    {
        x = px[i];

        /* LINEAR */
        if (blookup == 0) 
        {
            if (lin_search(pxa, x, prev_idx,  n - 1, &klo))
            {
                khi = klo + 1;
                prev_idx = klo;
            }
            else /* The linear probe did not find our x-val */
            {
                klo = b_search(pxa, x, 0, n - 1);
                khi = klo + 1;
                prev_idx = klo;
            }
        }
       /* BISECTION */
       else if (blookup == 1) 
       {
           if ( x >= pxa[prev_idx + 1] ) 
               prev_idx = b_search(pxa, x, prev_idx, n - 1);

           else if ( x < pxa[prev_idx] )
               prev_idx = b_search(pxa, x, 0, prev_idx);

        klo = prev_idx;
        khi = klo + 1;
       }

       /* COMPUTE */
        h     = pxa[khi] - pxa[klo];
        a     = (pxa[khi] - px[i]) / h;
        b     = (px[i] - pxa[klo]) / h;
        py[i] = a * pya[klo] + b * pya[khi] + ((a*a*a -a) * py2a[klo] + (b*b*b - b) * py2a[khi]) * h*h / 6.0;
    }
    return 0;
}