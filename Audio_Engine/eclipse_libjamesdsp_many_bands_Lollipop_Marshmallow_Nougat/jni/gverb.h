#ifndef GVERB_H
#define GVERB_H
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include "gverb.h"
typedef union
{
    float f;
    int32_t i;
} ls_pcast32;
long int lrintf(float x);
static __inline float flush_to_zero(float f)
{
    ls_pcast32 v;
    v.f = f;
    return (v.i & 0x7f800000) < 0x08000000 ? 0.0f : f;
}
#ifdef HAVE_LRINTF
#define f_round(f) lrintf(f)
#else
static __inline int f_round(float f)
{
    ls_pcast32 p;
    p.f = f;
    p.f += (3 << 22);
    return p.i - 0x4b400000;
}
#endif
typedef struct
{
    int size;
    int idx;
    float *buf;
} ty_fixeddelay;
typedef struct
{
    int size;
    float coeff;
    int idx;
    float *buf;
} ty_diffuser;
typedef struct
{
    float damping;
    float delay;
} ty_damper;
ty_diffuser *diffuser_make(int, float);
void diffuser_free(ty_diffuser *);
void diffuser_flush(ty_diffuser *);
ty_damper *damper_make(float);
void damper_free(ty_damper *);
void damper_flush(ty_damper *);
ty_fixeddelay *fixeddelay_make(int);
void fixeddelay_free(ty_fixeddelay *);
void fixeddelay_flush(ty_fixeddelay *);
int isprime(int);
int nearest_prime(int, float);
static __inline float diffuser_do(ty_diffuser *p, float x)
{
    float y, w;
    w = x - p->buf[p->idx] * p->coeff;
    w = flush_to_zero(w);
    y = p->buf[p->idx] + w*p->coeff;
    p->buf[p->idx] = w;
    p->idx = (p->idx + 1) % p->size;
    return(y);
}
static __inline float fixeddelay_read(ty_fixeddelay *p, int n)
{
    int i;
    i = (p->idx - n + p->size) % p->size;
    return(p->buf[i]);
}
static __inline void fixeddelay_write(ty_fixeddelay *p, float x)
{
    p->buf[p->idx] = x;
    p->idx = (p->idx + 1) % p->size;
}
static __inline void damper_set(ty_damper *p, float damping)
{
    p->damping = damping;
}
static __inline float damper_do(ty_damper *p, float x)
{
    float y;
    y = x*(1.0 - p->damping) + p->delay*p->damping;
    p->delay = y;
    return(y);
}
#define FDNORDER 4
typedef struct
{
    int rate;
    float inputbandwidth;
    float taillevel;
    float earlylevel;
    ty_damper *inputdamper;
    float maxroomsize;
    float roomsize;
    float revtime;
    float maxdelay;
    float largestdelay;
    ty_fixeddelay **fdndels;
    float *fdngains;
    int *fdnlens;
    ty_damper **fdndamps;
    float fdndamping;
    ty_diffuser **ldifs;
    ty_diffuser **rdifs;
    ty_fixeddelay *tapdelay;
    int *taps;
    float *tapgains;
    float *d;
    float *u;
    float *f;
    double alpha;
} ty_gverb;
ty_gverb *gverb_new(int, float, float, float, float, float, float, float, float);
void gverb_free(ty_gverb *);
void gverb_flush(ty_gverb *);
static void gverb_do(ty_gverb *, float, float *, float *);
static void gverb_set_roomsize(ty_gverb *, float);
static void gverb_set_revtime(ty_gverb *, float);
static void gverb_set_damping(ty_gverb *, float);
static void gverb_set_inputbandwidth(ty_gverb *, float);
static void gverb_set_earlylevel(ty_gverb *, float);
static void gverb_set_taillevel(ty_gverb *, float);
static __inline void gverb_fdnmatrix(float *a, float *b)
{
    const float dl0 = a[0], dl1 = a[1], dl2 = a[2], dl3 = a[3];
    b[0] = 0.5f*(+dl0 + dl1 - dl2 - dl3);
    b[1] = 0.5f*(+dl0 - dl1 - dl2 + dl3);
    b[2] = 0.5f*(-dl0 + dl1 - dl2 + dl3);
    b[3] = 0.5f*(+dl0 + dl1 + dl2 + dl3);
}
static __inline void gverb_do(ty_gverb *p, float x, float *yl, float *yr)
{
    float z;
    unsigned int i;
    float lsum,rsum,sum,sign;
    z = damper_do(p->inputdamper, x);
    z = diffuser_do(p->ldifs[0],z);
    for(i = 0; i < FDNORDER; i++)
        p->u[i] = p->tapgains[i]*fixeddelay_read(p->tapdelay,p->taps[i]);
    fixeddelay_write(p->tapdelay,z);
    for(i = 0; i < FDNORDER; i++)
    {
        p->d[i] = damper_do(p->fdndamps[i],
                            p->fdngains[i]*fixeddelay_read(p->fdndels[i], p->fdnlens[i]));
    }
    sum = 0.0f;
    sign = 1.0f;
    for(i = 0; i < FDNORDER; i++)
    {
        sum += sign*(p->taillevel*p->d[i] + p->earlylevel*p->u[i]);
        sign = -sign;
    }
    sum += x*p->earlylevel;
    lsum = sum;
    rsum = sum;
    gverb_fdnmatrix(p->d,p->f);
    for(i = 0; i < FDNORDER; i++)
        fixeddelay_write(p->fdndels[i],p->u[i]+p->f[i]);
    lsum = diffuser_do(p->ldifs[1],lsum);
    lsum = diffuser_do(p->ldifs[2],lsum);
    lsum = diffuser_do(p->ldifs[3],lsum);
    rsum = diffuser_do(p->rdifs[1],rsum);
    rsum = diffuser_do(p->rdifs[2],rsum);
    rsum = diffuser_do(p->rdifs[3],rsum);
    *yl = lsum;
    *yr = rsum;
}
static __inline void gverb_set_roomsize(ty_gverb *p, const float a)
{
    unsigned int i;
    if (a <= 1.0 || (a != a))
        p->roomsize = 1.0;
    else
        p->roomsize = a;
    p->largestdelay = p->rate * p->roomsize * 0.00294f;
    p->fdnlens[0] = f_round(1.000000f*p->largestdelay);
    p->fdnlens[1] = f_round(0.816490f*p->largestdelay);
    p->fdnlens[2] = f_round(0.707100f*p->largestdelay);
    p->fdnlens[3] = f_round(0.632450f*p->largestdelay);
    for(i = 0; i < FDNORDER; i++)
        p->fdngains[i] = -powf((float)p->alpha, p->fdnlens[i]);
    p->taps[0] = 5+f_round(0.410f*p->largestdelay);
    p->taps[1] = 5+f_round(0.300f*p->largestdelay);
    p->taps[2] = 5+f_round(0.155f*p->largestdelay);
    p->taps[3] = 5+f_round(0.000f*p->largestdelay);
    for(i = 0; i < FDNORDER; i++)
        p->tapgains[i] = powf((float)p->alpha, p->taps[i]);
}
static __inline void gverb_set_revtime(ty_gverb *p,float a)
{
    float ga,gt;
    double n;
    unsigned int i;
    p->revtime = a;
    ga = 60.0;
    gt = p->revtime;
    ga = powf(10.0f,-ga/20.0f);
    n = p->rate*gt;
    p->alpha = (double)powf(ga,1.0f/n);
    for(i = 0; i < FDNORDER; i++)
        p->fdngains[i] = -powf((float)p->alpha, p->fdnlens[i]);
}
static __inline void gverb_set_damping(ty_gverb *p,float a)
{
    unsigned int i;
    p->fdndamping = a;
    for(i = 0; i < FDNORDER; i++)
        damper_set(p->fdndamps[i],p->fdndamping);
}
static __inline void gverb_set_inputbandwidth(ty_gverb *p,float a)
{
    p->inputbandwidth = a;
    damper_set(p->inputdamper,1.0 - p->inputbandwidth);
}
static __inline void gverb_set_earlylevel(ty_gverb *p,float a)
{
    p->earlylevel = a;
}
static __inline void gverb_set_taillevel(ty_gverb *p,float a)
{
    p->taillevel = a;
}
#endif
