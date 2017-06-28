#ifndef DSPFILTERS_ROOTFINDER_H
#define DSPFILTERS_ROOTFINDER_H
#include "Common.h"
#include "Types.h"
namespace Iir
{
class RootFinderBase
{
public:
    struct Array
    {
        Array (int max, complex_t* values)
        {
        }
    };
    void solve (int degree,
                bool polish = true,
                bool doSort = true);
    complex_t eval (int degree,
                    const complex_t& x);
    complex_t* coef()
    {
        return m_a;
    }
    complex_t* root()
    {
        return m_root;
    }
    void sort (int degree);
private:
    void laguerre (int degree,
                   complex_t a[],
                   complex_t& x,
                   int& its);
protected:
    int m_maxdegree;
    complex_t* m_a;
    complex_t* m_ad;
    complex_t* m_root;
};
template<int maxdegree>
struct RootFinder : RootFinderBase
{
    RootFinder()
    {
        m_maxdegree = maxdegree;
        m_a = m_a0;
        m_ad = m_ad0;
        m_root = m_r;
    }
private:
    complex_t m_a0 [maxdegree+1];
    complex_t m_ad0[maxdegree+1];
    complex_t m_r [maxdegree];
};
}
#endif
