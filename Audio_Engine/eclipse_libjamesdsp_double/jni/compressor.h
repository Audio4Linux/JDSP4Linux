// (c) Copyright 2016, Sean Connelly (@voidqk), http://syntheti.cc
// MIT License
// Project Home: https://github.com/voidqk/sndfilter

// dynamics compressor based on WebAudio specification:
//   https://webaudio.github.io/web-audio-api/#the-dynamicscompressornode-interface

#ifndef SNDFILTER_COMPRESSOR__H
#define SNDFILTER_COMPRESSOR__H

// dynamic range compression is a complex topic with many different algorithms
//
// this API works by first initializing an sf_compressor_state_st structure, then using it to
// process a sample in chunks
//
// for example, say you're processing a stream in 128 samples per chunk:
//
//   sf_compressor_state_st simplecomp;
//   sf_simplecomp(&simplecomp, 48000, 5, -24, 30, 12, 0.003f, 0.250f);
//
//   for each 128 length sample:
//     sf_compressor_process(&simplecomp, 128, input, output);
//
// notice that sf_compressor_process will change a lot of the member variables inside of the state
// structure, since these values must be carried over across chunk boundaries
//
// also notice that the choice to divide the sound into chunks of 128 samples is completely
// arbitrary from the compressor's perspective, however, the size should be divisible by the SPU
// value below (defaults to 32):

// maximum number of samples in the delay buffer
#define SF_COMPRESSOR_MAXDELAY   256

// samples per update; the compressor works by dividing the input chunks into even smaller sizes,
// and performs heavier calculations after each mini-chunk to adjust the final envelope
#define SF_COMPRESSOR_SPU        8

// not sure what this does exactly, but it is part of the release curve
#define SF_COMPRESSOR_SPACINGDB  5.0

typedef struct
{
#ifdef METER
    // user can read the metergain state variable after processing a chunk to see how much dB the
    // compressor would have liked to compress the sample; the meter values aren't used to shape the
    // sound in any way, only used for output if desired
    double metergain;
    double meterrelease;
#endif
    // everything else shouldn't really be mucked with unless you read the algorithm and feel
    // comfortable
    double threshold;
    double knee;
    double linearpregain;
    double linearthreshold;
    double slope;
    double attacksamplesinv;
    double satreleasesamplesinv;
    double k;
    double kneedboffset;
    double linearthresholdknee;
    double mastergain;
    double a; // adaptive release polynomial coefficients
    double b;
    double c;
    double d;
    double detectoravg;
    double compgain;
    double maxcompdiffdb;
    int delaybufsize;
    int delaywritepos;
    int delayreadpos;
    double delaybufL[SF_COMPRESSOR_MAXDELAY]; // predelay buffer
    double delaybufR[SF_COMPRESSOR_MAXDELAY]; // predelay buffer
} sf_compressor_state_st;

// populate a compressor state with advanced parameters
void sf_advancecomp(sf_compressor_state_st *state,
                    // these parameters are the same as the simple version above:
                    int rate, double pregain, double threshold, double knee, double ratio, double attack, double release,
                    // these are the advanced parameters:
                    double predelay,     // seconds, length of the predelay buffer [0 to 1]
                    double releasezone1, // release zones should be increasing between 0 and 1, and are a fraction
                    double releasezone2, //  of the release time depending on the input dB -- these parameters define
                    double releasezone3, //  the adaptive release curve, which is discussed in further detail in the
                    double releasezone4, //  demo: adaptive-release-curve.html
                    double postgain     // dB, amount of gain to apply after compression [0 to 100]
                   );

// this function will process the input sound based on the state passed
// the input and output buffers should be the same size
void sf_compressor_process(sf_compressor_state_st *state, int size, double *inputL, double *inputR, double *outputL, double *outputR);

#endif // SNDFILTER_COMPRESSOR__H