#ifndef DSPFILTERS_TYPES_H
#define DSPFILTERS_TYPES_H
#include "Common.h"
#include "MathSupplement.h"
namespace Iir
{
struct ComplexPair : complex_pair_t
{
    ComplexPair ()
    {
    }
    explicit ComplexPair (const complex_t& c1)
        : complex_pair_t (c1, 0.)
    {
    }
    ComplexPair (const complex_t& c1,
                 const complex_t& c2)
        : complex_pair_t (c1, c2)
    {
    }
};
struct PoleZeroPair
{
    ComplexPair poles;
    ComplexPair zeros;
    PoleZeroPair () { }
    PoleZeroPair (const complex_t& p, const complex_t& z)
        : poles (p), zeros (z)
    {
    }
    PoleZeroPair (const complex_t& p1, const complex_t& z1,
                  const complex_t& p2, const complex_t& z2)
        : poles (p1, p2)
        , zeros (z1, z2)
    {
    }
    bool isSinglePole () const
    {
        return poles.second == 0. && zeros.second == 0.;
    }
};
}
#endif
