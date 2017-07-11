#include "gverb.h"
ty_diffuser *diffuser_make(int size, float coeff)
{
    ty_diffuser *p;
    int i;
    p = (ty_diffuser *)malloc(sizeof(ty_diffuser));
    p->size = size;
    p->coeff = coeff;
    p->idx = 0;
    p->buf = (float *)malloc(size * sizeof(float));
    for (i = 0; i < size; i++) p->buf[i] = 0.0;
    return(p);
}
ty_damper *damper_make(float damping)
{
    ty_damper *p;
    p = (ty_damper *)malloc(sizeof(ty_damper));
    p->damping = damping;
    p->delay = 0.0f;
    return(p);
}
ty_fixeddelay *fixeddelay_make(int size)
{
    ty_fixeddelay *p;
    int i;
    p = (ty_fixeddelay *)malloc(sizeof(ty_fixeddelay));
    p->size = size;
    p->idx = 0;
    p->buf = (float *)malloc(size * sizeof(float));
    for (i = 0; i < size; i++) p->buf[i] = 0.0;
    return(p);
}
ty_gverb *gverb_new(int srate, float maxroomsize, float roomsize,
                    float revtime, float damping, float spread,
                    float inputbandwidth, float earlylevel, float taillevel)
{
    ty_gverb *p;
    float ga,gb,gt;
    int i,n;
    float r;
    float diffscale;
    int a,b,c,cc,d,dd,e;
    float spread1,spread2;
    p = (ty_gverb *)malloc(sizeof(ty_gverb));
    p->rate = srate;
    p->fdndamping = damping;
    p->maxroomsize = maxroomsize;
    p->roomsize = roomsize;
    p->revtime = revtime;
    p->earlylevel = earlylevel;
    p->taillevel = taillevel;
    p->maxdelay = p->rate*p->maxroomsize/340.0;
    p->largestdelay = p->rate*p->roomsize/340.0;
    p->inputbandwidth = inputbandwidth;
    p->inputdamper = damper_make(1.0 - p->inputbandwidth);
    p->fdndels = (ty_fixeddelay **)calloc(FDNORDER, sizeof(ty_fixeddelay *));
    for(i = 0; i < FDNORDER; i++)
        p->fdndels[i] = fixeddelay_make((int)p->maxdelay+1000);
    p->fdngains = (float *)calloc(FDNORDER, sizeof(float));
    p->fdnlens = (int *)calloc(FDNORDER, sizeof(int));
    p->fdndamps = (ty_damper **)calloc(FDNORDER, sizeof(ty_damper *));
    for(i = 0; i < FDNORDER; i++)
        p->fdndamps[i] = damper_make(p->fdndamping);
    ga = 60.0;
    gt = p->revtime;
    ga = powf(10.0f,-ga/20.0f);
    n = p->rate*gt;
    p->alpha = pow((double)ga, 1.0/(double)n);
    gb = 0.0;
    for(i = 0; i < FDNORDER; i++)
    {
        if (i == 0) gb = 1.000000*p->largestdelay;
        if (i == 1) gb = 0.816490*p->largestdelay;
        if (i == 2) gb = 0.707100*p->largestdelay;
        if (i == 3) gb = 0.632450*p->largestdelay;
        p->fdnlens[i] = f_round(gb);
        p->fdngains[i] = -powf((float)p->alpha,p->fdnlens[i]);
    }
    p->d = (float *)calloc(FDNORDER, sizeof(float));
    p->u = (float *)calloc(FDNORDER, sizeof(float));
    p->f = (float *)calloc(FDNORDER, sizeof(float));
    diffscale = (float)p->fdnlens[3]/(210+159+562+410);
    spread1 = spread;
    spread2 = 3.0*spread;
    b = 210;
    r = 0.125541;
    a = spread1*r;
    c = 210+159+a;
    cc = c-b;
    r = 0.854046;
    a = spread2*r;
    d = 210+159+562+a;
    dd = d-c;
    e = 1341-d;
    p->ldifs = (ty_diffuser **)calloc(4, sizeof(ty_diffuser *));
    p->ldifs[0] = diffuser_make((int)(diffscale*b),0.75);
    p->ldifs[1] = diffuser_make((int)(diffscale*cc),0.75);
    p->ldifs[2] = diffuser_make((int)(diffscale*dd),0.625);
    p->ldifs[3] = diffuser_make((int)(diffscale*e),0.625);
    b = 210;
    r = -0.568366;
    a = spread1*r;
    c = 210+159+a;
    cc = c-b;
    r = -0.126815;
    a = spread2*r;
    d = 210+159+562+a;
    dd = d-c;
    e = 1341-d;
    p->rdifs = (ty_diffuser **)calloc(4, sizeof(ty_diffuser *));
    p->rdifs[0] = diffuser_make((int)(diffscale*b),0.75);
    p->rdifs[1] = diffuser_make((int)(diffscale*cc),0.75);
    p->rdifs[2] = diffuser_make((int)(diffscale*dd),0.625);
    p->rdifs[3] = diffuser_make((int)(diffscale*e),0.625);
    p->tapdelay = fixeddelay_make(44000);
    p->taps = (int *)calloc(FDNORDER, sizeof(int));
    p->tapgains = (float *)calloc(FDNORDER, sizeof(float));
    p->taps[0] = 5+0.410*p->largestdelay;
    p->taps[1] = 5+0.300*p->largestdelay;
    p->taps[2] = 5+0.155*p->largestdelay;
    p->taps[3] = 5+0.000*p->largestdelay;
    for(i = 0; i < FDNORDER; i++)
        p->tapgains[i] = pow(p->alpha,(double)p->taps[i]);
    return(p);
}
void damper_free(ty_damper *p)
{
    free(p);
}
void damper_flush(ty_damper *p)
{
    p->delay = 0.0f;
}
void diffuser_free(ty_diffuser *p)
{
    free(p->buf);
    free(p);
}
void fixeddelay_free(ty_fixeddelay *p)
{
    free(p->buf);
    free(p);
}
void fixeddelay_flush(ty_fixeddelay *p)
{
    memset(p->buf, 0, p->size * sizeof(float));
}
void diffuser_flush(ty_diffuser *p)
{
    memset(p->buf, 0, p->size * sizeof(float));
}
void gverb_free(ty_gverb *p)
{
    int i;
    damper_free(p->inputdamper);
    for(i = 0; i < FDNORDER; i++)
    {
        fixeddelay_free(p->fdndels[i]);
        damper_free(p->fdndamps[i]);
        diffuser_free(p->ldifs[i]);
        diffuser_free(p->rdifs[i]);
    }
    free(p->fdndels);
    free(p->fdngains);
    free(p->fdnlens);
    free(p->fdndamps);
    free(p->d);
    free(p->u);
    free(p->f);
    free(p->ldifs);
    free(p->rdifs);
    free(p->taps);
    free(p->tapgains);
    fixeddelay_free(p->tapdelay);
    free(p);
}
void gverb_flush(ty_gverb *p)
{
    int i;
    damper_flush(p->inputdamper);
    for(i = 0; i < FDNORDER; i++)
    {
        fixeddelay_flush(p->fdndels[i]);
        damper_flush(p->fdndamps[i]);
        diffuser_flush(p->ldifs[i]);
        diffuser_flush(p->rdifs[i]);
    }
    memset(p->d, 0, FDNORDER * sizeof(float));
    memset(p->u, 0, FDNORDER * sizeof(float));
    memset(p->f, 0, FDNORDER * sizeof(float));
    fixeddelay_flush(p->tapdelay);
}
