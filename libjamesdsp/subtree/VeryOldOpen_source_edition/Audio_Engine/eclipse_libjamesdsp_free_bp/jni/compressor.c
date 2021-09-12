// (c) Copyright 2016, Sean Connelly (@voidqk), http://syntheti.cc
// MIT License
// Project Home: https://github.com/voidqk/sndfilter

#include "compressor.h"
#include <math.h>
#include <string.h>

// core algorithm extracted from Chromium source, DynamicsCompressorKernel.cpp, here:
//   https://git.io/v1uSK
//
// changed a few things though in an attempt to simplify the curves and algorithm, and also included
// a pregain so that samples can be scaled up then compressed

static inline double db2lin(double db)  // dB to linear
{
    return pow(10.0, 0.05 * db);
}

static inline double lin2db(double lin)  // linear to dB
{
    return 20.0 * log10(lin);
}

// for more information on the knee curve, check out the compressor-curve.html demo + source code
// included in this repo
static inline double kneecurve(double x, double k, double linearthreshold)
{
    return linearthreshold + (1.0 - exp(-k * (x - linearthreshold))) / k;
}

double kneeslope(double x, double k, double linearthreshold)
{
    return k * x / ((k * linearthreshold + 1.0) * exp(k * (x - linearthreshold)) - 1.0);
}

static inline double compcurve(double x, double k, double slope, double linearthreshold,
                              double linearthresholdknee, double threshold, double knee, double kneedboffset)
{
    if (x < linearthreshold)
        return x;
    if (knee <= 0.0) // no knee in curve
        return db2lin(threshold + slope * (lin2db(x) - threshold));
    if (x < linearthresholdknee)
        return kneecurve(x, k, linearthreshold);
    return db2lin(kneedboffset + slope * (lin2db(x) - threshold - knee));
}

// this is the main initialization function
// it does a bunch of pre-calculation so that the inner loop of signal processing is fast
void sf_advancecomp(sf_compressor_state_st *state, int rate, double pregain, double threshold,
                    double knee, double ratio, double attack, double release, double predelay, double releasezone1,
                    double releasezone2, double releasezone3, double releasezone4, double postgain)
{
    // setup the predelay buffer
    int delaybufsize = rate * predelay;
    if (delaybufsize < 1)
        delaybufsize = 1;
    else if (delaybufsize > SF_COMPRESSOR_MAXDELAY)
        delaybufsize = SF_COMPRESSOR_MAXDELAY;
    if (delaybufsize > 0)
    {
        memset(state->delaybufL, 0, sizeof(double) * delaybufsize);
        memset(state->delaybufR, 0, sizeof(double) * delaybufsize);
    }
    // useful values
    double linearpregain = db2lin(pregain);
    double linearthreshold = db2lin(threshold);
    double slope = 1.0 / ratio;
    double attacksamples = rate * attack;
    double attacksamplesinv = 1.0 / attacksamples;
    double releasesamples = rate * release;
    double satrelease = 0.0025; // seconds
    double satreleasesamplesinv = 1.0 / ((double)rate * satrelease);
#ifdef METER
    // metering values (not used in core algorithm, but used to output a meter if desired)
    double metergain = 1.0; // gets overwritten immediately because gain will always be negative
    double meterfalloff = 0.325; // seconds
    double meterrelease = 1.0 - exp(-1.0 / ((double)rate * meterfalloff));
#endif
    // calculate knee curve parameters
    double k = 5.0; // initial guess
    double kneedboffset;
    double linearthresholdknee;
    if (knee > 0.0)  // if a knee exists, search for a good k value
    {
        double xknee = db2lin(threshold + knee);
        double mink = 0.1;
        double maxk = 10000.0;
        // search by comparing the knee slope at the current k guess, to the ideal slope
        for (int i = 0; i < 15; i++)
        {
            if (kneeslope(xknee, k, linearthreshold) < slope)
                maxk = k;
            else
                mink = k;
            k = sqrtf(mink * maxk);
        }
        kneedboffset = lin2db(kneecurve(xknee, k, linearthreshold));
        linearthresholdknee = db2lin(threshold + knee);
    }
    // calculate a master gain based on what sounds good
    double fulllevel = compcurve(1.0, k, slope, linearthreshold, linearthresholdknee,
                                threshold, knee, kneedboffset);
    double mastergain = db2lin(postgain) * pow(1.0 / fulllevel, 0.6);
    // calculate the adaptive release curve parameters
    // solve a,b,c,d in `y = a*x^3 + b*x^2 + c*x + d`
    // interescting points (0, y1), (1, y2), (2, y3), (3, y4)
    double y1 = releasesamples * releasezone1;
    double y2 = releasesamples * releasezone2;
    double y3 = releasesamples * releasezone3;
    double y4 = releasesamples * releasezone4;
    double a = (-y1 + 3.0 * y2 - 3.0 * y3 + y4) / 6.0;
    double b = y1 - 2.5 * y2 + 2.0 * y3 - 0.5 * y4;
    double c = (-11.0 * y1 + 18.0 * y2 - 9.0 * y3 + 2.0 * y4) / 6.0;
    double d = y1;
    // save everything
#ifdef METER
    state->metergain            = 1.0; // large value overwritten immediately since it's always < 0
    state->meterrelease         = meterrelease;
#endif
    state->threshold            = threshold;
    state->knee                 = knee;
    state->linearpregain        = linearpregain;
    state->linearthreshold      = linearthreshold;
    state->slope                = slope;
    state->attacksamplesinv     = attacksamplesinv;
    state->satreleasesamplesinv = satreleasesamplesinv;
    state->k                    = k;
    state->kneedboffset         = kneedboffset;
    state->linearthresholdknee  = linearthresholdknee;
    state->mastergain           = mastergain;
    state->a                    = a;
    state->b                    = b;
    state->c                    = c;
    state->d                    = d;
    state->detectoravg          = 0.0;
    state->compgain             = 1.0;
    state->maxcompdiffdb        = -1.0;
    state->delaybufsize         = delaybufsize;
    state->delaywritepos        = 0;
    state->delayreadpos         = delaybufsize > 1 ? 1 : 0;
}

// for more information on the adaptive release curve, check out adaptive-release-curve.html demo +
// source code included in this repo
static inline double adaptivereleasecurve(double x, double a, double b, double c, double d)
{
    // a*x^3 + b*x^2 + c*x + d
    double x2 = x * x;
    return a * x2 * x + b * x2 + c * x + d;
}

static inline double clampf(double v, double min, double max)
{
    return v < min ? min : (v > max ? max : v);
}

static inline double absf(double v)
{
    return v < 0.0 ? -v : v;
}

static inline double fixf(double v, double def)
{
    // fix NaN and infinity values that sneak in... not sure why this is needed, but it is
    if (isnan(v) || isinf(v))
        return def;
    return v;
}

void sf_compressor_process(sf_compressor_state_st *state, int size, double *inputL, double *inputR, double *outputL, double *outputR)
{
    // pull out the state into local variables
#ifdef METER
    double metergain            = state->metergain;
    double meterrelease         = state->meterrelease;
#endif
    double threshold            = state->threshold;
    double knee                 = state->knee;
    double linearpregain        = state->linearpregain;
    double linearthreshold      = state->linearthreshold;
    double slope                = state->slope;
    double attacksamplesinv     = state->attacksamplesinv;
    double satreleasesamplesinv = state->satreleasesamplesinv;
    double k                    = state->k;
    double kneedboffset         = state->kneedboffset;
    double linearthresholdknee  = state->linearthresholdknee;
    double mastergain           = state->mastergain;
    double a                    = state->a;
    double b                    = state->b;
    double c                    = state->c;
    double d                    = state->d;
    double detectoravg          = state->detectoravg;
    double compgain             = state->compgain;
    double maxcompdiffdb        = state->maxcompdiffdb;
    int delaybufsize           = state->delaybufsize;
    int delaywritepos          = state->delaywritepos;
    int delayreadpos           = state->delayreadpos;
    double *delaybufL           = state->delaybufL;
    double *delaybufR           = state->delaybufR;
    int chunks = size / SF_COMPRESSOR_SPU;
    double ang90 = 1.570796326794897;
    double ang90inv = 0.636619772367581;
    int samplepos = 0;
    for (int ch = 0; ch < chunks; ch++)
    {
        detectoravg = fixf(detectoravg, 1.0);
        double desiredgain = detectoravg;
        double scaleddesiredgain = asin(desiredgain) * ang90inv;
        double compdiffdb = lin2db(compgain / scaleddesiredgain);
        // calculate envelope rate based on whether we're attacking or releasing
        double enveloperate;
        if (compdiffdb < 0.0)  // compgain < scaleddesiredgain, so we're releasing
        {
            compdiffdb = fixf(compdiffdb, -1.0);
            maxcompdiffdb = -1; // reset for a future attack mode
            // apply the adaptive release curve
            // scale compdiffdb between 0-3
            double x = (clampf(compdiffdb, -12.0, 0.0) + 12.0) * 0.25;
            double releasesamples = adaptivereleasecurve(x, a, b, c, d);
            enveloperate = db2lin(SF_COMPRESSOR_SPACINGDB / releasesamples);
        }
        else  // compresorgain > scaleddesiredgain, so we're attacking
        {
            compdiffdb = fixf(compdiffdb, 1.0);
            if (maxcompdiffdb == -1 || maxcompdiffdb < compdiffdb)
                maxcompdiffdb = compdiffdb;
            double attenuate = maxcompdiffdb;
            if (attenuate < 0.5)
                attenuate = 0.5;
            enveloperate = 1.0 - pow(0.25 / attenuate, attacksamplesinv);
        }
        // process the chunk
        for (int chi = 0; chi < SF_COMPRESSOR_SPU; chi++, samplepos++,
                delayreadpos = (delayreadpos + 1) % delaybufsize,
                delaywritepos = (delaywritepos + 1) % delaybufsize)
        {
            double inL = inputL[samplepos] * linearpregain;
            double inR = inputR[samplepos] * linearpregain;
            delaybufL[delaywritepos] = inL;
            delaybufR[delaywritepos] = inR;
            inL = absf(inL);
            inR = absf(inR);
            double inputmax = inL > inR ? inL : inR;
            double attenuation;
            if (inputmax < 0.0001)
                attenuation = 1.0;
            else
            {
                double inputcomp = compcurve(inputmax, k, slope, linearthreshold,
                                            linearthresholdknee, threshold, knee, kneedboffset);
                attenuation = inputcomp / inputmax;
            }
            double rate;
            if (attenuation > detectoravg)  // if releasing
            {
                double attenuationdb = -lin2db(attenuation);
                if (attenuationdb < 2.0)
                    attenuationdb = 2.0;
                double dbpersample = attenuationdb * satreleasesamplesinv;
                rate = db2lin(dbpersample) - 1.0;
            }
            else
                rate = 1.0;
            detectoravg += (attenuation - detectoravg) * rate;
            if (detectoravg > 1.0)
                detectoravg = 1.0;
            detectoravg = fixf(detectoravg, 1.0);
            if (enveloperate < 1) // attack, reduce gain
                compgain += (scaleddesiredgain - compgain) * enveloperate;
            else  // release, increase gain
            {
                compgain *= enveloperate;
                if (compgain > 1.0)
                    compgain = 1.0;
            }
            // the final gain value!
            double premixgain = sin(ang90 * compgain);
            double gain = mastergain * premixgain;
#ifdef METER
            // calculate metering (not used in core algo, but used to output a meter if desired)
            double premixgaindb = lin2db(premixgain);
            if (premixgaindb < metergain)
                metergain = premixgaindb; // spike immediately
            else
                metergain += (premixgaindb - metergain) * meterrelease; // fall slowly
#endif
            // apply the gain
            outputL[samplepos] = delaybufL[delayreadpos] * gain;
            outputR[samplepos] = delaybufR[delayreadpos] * gain;
        }
    }
#ifdef METER
    state->metergain     = metergain;
#endif
    state->detectoravg   = detectoravg;
    state->compgain      = compgain;
    state->maxcompdiffdb = maxcompdiffdb;
    state->delaywritepos = delaywritepos;
    state->delayreadpos  = delayreadpos;
}