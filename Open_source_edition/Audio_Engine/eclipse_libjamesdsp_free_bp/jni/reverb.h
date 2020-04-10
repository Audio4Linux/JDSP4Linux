// (c) Copyright 2016, Sean Connelly (@voidqk), http://syntheti.cc
// MIT License
// Project Home: https://github.com/voidqk/sndfilter

// reverb algorithm is based on Progenitor2 reverb effect from freeverb3:
//   http://www.nongnu.org/freeverb3/

#ifndef SNDFILTER_REVERB__H
#define SNDFILTER_REVERB__H
// this API works by first initializing an sf_reverb_state_st structure, then using it to process a
// sample in chunks
//
//   sf_reverb_state_st rv;
//   sf_presetreverb(&rv, 44100, SF_REVERB_PRESET_DEFAULT);
//
//   for each abitrary length sample:
//   double outputL, outputR;
//   sf_reverb_process(&rv, inputL, inputR, &outputL, &outputR);
//
// notice that sf_reverb_process will change a lot of the member variables inside of the state
// structure, since these values must be carried over across chunk boundaries
//
// ---
//
// non-convolution based reverb effects are made up from a lot of smaller effects
//
// each reverb algorithm's sound is based on how the designers setup these smaller effects and
// chained them together
//
// this particular setup is based on Progenitor2 from Freeverb3, and uses the following components:
//    1. Delay
//    2. 1st order IIR filter (lowpass filter, highpass filter)
//    3. Biquad filter (lowpass filter, all-pass filter)
//    4. Early reflection
//    5. Oversampling
//    6. DC cut
//    7. Fractal noise
//    8. Low-frequency oscilator (LFO)
//    9. All-pass filter
//   10. 2nd order All-pass filter
//   11. 3rd order All-pass filter with modulation
//   12. Modulated all-pass filter
//   13. Delayed feedforward comb filter
//
// each of these components is broken into their own structures (sf_rv_*), and the reverb effect
// uses these in the final state structure (sf_reverb_state_st)
//
// each component is designed to work one step at a time, so any size sample can be streamed through
// in one pass

// delay
// delay buffer size; maximum size allowed for a delay
#define SF_REVERB_DS        3000
typedef struct
{
    int pos;                 // current write position
    int size;                // delay size
    double buf[SF_REVERB_DS]; // delay buffer
} sf_rv_delay_st;

// 1st order IIR filter
typedef struct
{
    double a2; // coefficients
    double b1;
    double b2;
    double y1; // state
} sf_rv_iir1_st;

// biquad
// note: we don't use biquad.c because we want to step through the sound one sample at a time, one
//       channel at a time
typedef struct
{
    double b0; // biquad coefficients
    double b1;
    double b2;
    double a1;
    double a2;
    double xn1; // input[n - 1]
    double xn2; // input[n - 2]
    double yn1; // output[n - 1]
    double yn2; // output[n - 2]
} sf_rv_biquad_st;

// early reflection
typedef struct
{
    int             delaytblL[18], delaytblR[18];
    sf_rv_delay_st  delayPWL, delayPWR;
    sf_rv_delay_st  delayRL, delayLR;
    sf_rv_biquad_st allpassXL, allpassXR;
    sf_rv_biquad_st allpassL, allpassR;
    sf_rv_iir1_st   lpfL, lpfR;
    sf_rv_iir1_st   hpfL, hpfR;
    double wet1, wet2;
} sf_rv_earlyref_st;

// oversampling
// maximum oversampling factor
#define SF_REVERB_OF        2
typedef struct
{
    int factor;           // oversampling factor [1 to SF_REVERB_OF]
    sf_rv_biquad_st lpfU; // lowpass filter used for upsampling
    sf_rv_biquad_st lpfD; // lowpass filter used for downsampling
} sf_rv_oversample_st;

// dc cut
typedef struct
{
    double gain;
    double y1;
    double y2;
} sf_rv_dccut_st;

// fractal noise cache
// noise buffer size; must be a power of 2 because it's generated via fractal generator
#define SF_REVERB_NS        (1<<11)
typedef struct
{
    int pos;                 // current read position in the buffer
    double buf[SF_REVERB_NS]; // buffer filled with noise
} sf_rv_noise_st;

// low-frequency oscilator (LFO)
typedef struct
{
    double re;  // real part
    double im;  // imaginary part
    double sn;  // sin of angle increment per sample
    double co;  // cos of angle increment per sample
    int count; // number of samples generated so far (used to apply small corrections over time)
} sf_rv_lfo_st;

// all-pass filter
// maximum size
#define SF_REVERB_APS       3400
typedef struct
{
    int pos;
    int size;
    double feedback;
    double decay;
    double buf[SF_REVERB_APS];
} sf_rv_allpass_st;

// 2nd order all-pass filter
// maximum sizes of the two buffers
#define SF_REVERB_AP2S1     4200
#define SF_REVERB_AP2S2     3000
typedef struct
{
    //    line 1                 line 2
    int   pos1, pos2;
    int   size1, size2;
    double feedback1, feedback2;
    double decay1, decay2;
    double buf1[SF_REVERB_AP2S1], buf2[SF_REVERB_AP2S2];
} sf_rv_allpass2_st;

// 3rd order all-pass filter with modulation
// maximum sizes of the three buffers and maximum mod size of the first line
#define SF_REVERB_AP3S1     4000
#define SF_REVERB_AP3M1     600
#define SF_REVERB_AP3S2     2000
#define SF_REVERB_AP3S3     3000
typedef struct
{
    //    line 1 (with modulation)                 line 2                 line 3
    int   rpos1, wpos1, pos2, pos3;
    int   size1, msize1, size2, size3;
    double feedback1, feedback2, feedback3;
    double decay1, decay2, decay3;
    double buf1[SF_REVERB_AP3S1 + SF_REVERB_AP3M1], buf2[SF_REVERB_AP3S2], buf3[SF_REVERB_AP3S3];
} sf_rv_allpass3_st;

// modulated all-pass filter
// maximum size and maximum mod size
#define SF_REVERB_APMS      3600
#define SF_REVERB_APMM      137
typedef struct
{
    int rpos, wpos;
    int size, msize;
    double feedback;
    double decay;
    double z1;
    double buf[SF_REVERB_APMS + SF_REVERB_APMM];
} sf_rv_allpassm_st;

// comb filter
// maximum size of the buffer
#define SF_REVERB_CS        1500
typedef struct
{
    int pos;
    int size;
    double buf[SF_REVERB_CS];
} sf_rv_comb_st;

//
// the final reverb state structure
//
// note: this struct is about 1Mb
typedef struct
{
    sf_rv_earlyref_st   earlyref;
    sf_rv_oversample_st oversampleL, oversampleR;
    sf_rv_dccut_st      dccutL, dccutR;
    sf_rv_noise_st      noise;
    sf_rv_lfo_st        lfo1;
    sf_rv_iir1_st       lfo1_lpf;
    sf_rv_allpassm_st   diffL[10], diffR[10];
    sf_rv_allpass_st    crossL[4], crossR[4];
    sf_rv_iir1_st       clpfL, clpfR; // cross LPF
    sf_rv_delay_st      cdelayL, cdelayR; // cross delay
    sf_rv_biquad_st     bassapL, bassapR; // bass all-pass
    sf_rv_biquad_st     basslpL, basslpR; // bass lowpass
    sf_rv_iir1_st       damplpL, damplpR; // dampening lowpass
    sf_rv_allpassm_st   dampap1L, dampap1R; // dampening all-pass (1)
    sf_rv_delay_st      dampdL, dampdR; // dampening delay
    sf_rv_allpassm_st   dampap2L, dampap2R; // dampening all-pass (2)
    sf_rv_delay_st      cbassd1L, cbassd1R; // cross-fade bass delay (1)
    sf_rv_allpass2_st   cbassap1L, cbassap1R; // cross-fade bass allpass (1)
    sf_rv_delay_st      cbassd2L, cbassd2R; // cross-fade bass delay (2)
    sf_rv_allpass3_st   cbassap2L, cbassap2R; // cross-fade bass allpass (2)
    sf_rv_lfo_st        lfo2;
    sf_rv_iir1_st       lfo2_lpf;
    sf_rv_comb_st       combL, combR;
    sf_rv_biquad_st     lastlpfL, lastlpfR;
    sf_rv_delay_st      lastdelayL, lastdelayR;
    sf_rv_delay_st      inpdelayL, inpdelayR;
    int outco[32];
    double loopdecay;
    double wet1, wet2;
    double wander;
    double bassb;
    double ertolate; // early reflection mix parameters
    double erefwet;
    double dry;
} sf_reverb_state_st;

typedef enum
{
    SF_REVERB_PRESET_DEFAULT,
    SF_REVERB_PRESET_SMALLHALL1,
    SF_REVERB_PRESET_SMALLHALL2,
    SF_REVERB_PRESET_MEDIUMHALL1,
    SF_REVERB_PRESET_MEDIUMHALL2,
    SF_REVERB_PRESET_LARGEHALL1,
    SF_REVERB_PRESET_LARGEHALL2,
    SF_REVERB_PRESET_SMALLROOM1,
    SF_REVERB_PRESET_SMALLROOM2,
    SF_REVERB_PRESET_MEDIUMROOM1,
    SF_REVERB_PRESET_MEDIUMROOM2,
    SF_REVERB_PRESET_LARGEROOM1,
    SF_REVERB_PRESET_LARGEROOM2,
    SF_REVERB_PRESET_MEDIUMER1,
    SF_REVERB_PRESET_MEDIUMER2,
    SF_REVERB_PRESET_PLATEHIGH,
    SF_REVERB_PRESET_PLATELOW,
    SF_REVERB_PRESET_LONGREVERB1,
    SF_REVERB_PRESET_LONGREVERB2
} sf_reverb_preset;

// populate a reverb state with a preset
void sf_presetreverb(sf_reverb_state_st *state, int rate, sf_reverb_preset preset);

// populate a reverb state with advanced parameters
void sf_advancereverb(sf_reverb_state_st *rv,
                      int rate,             // input sample rate (samples per second)
                      int oversamplefactor, // how much to oversample [1 to 4]
                      double ertolate,       // early reflection amount [0 to 1]
                      double erefwet,        // dB, final wet mix [-70 to 10]
                      double dry,            // dB, final dry mix [-70 to 10]
                      double ereffactor,     // early reflection factor [0.5 to 2.5]
                      double erefwidth,      // early reflection width [-1 to 1]
                      double width,          // width of reverb L/R mix [0 to 1]
                      double wet,            // dB, reverb wetness [-70 to 10]
                      double wander,         // LFO wander amount [0.1 to 0.6]
                      double bassb,          // bass boost [0 to 0.5]
                      double spin,           // LFO spin amount [0 to 10]
                      double inputlpf,       // Hz, lowpass cutoff for input [200 to 18000]
                      double basslpf,        // Hz, lowpass cutoff for bass [50 to 1050]
                      double damplpf,        // Hz, lowpass cutoff for dampening [200 to 18000]
                      double outputlpf,      // Hz, lowpass cutoff for output [200 to 18000]
                      double rt60,           // reverb time decay [0.1 to 30]
                      double delay          // seconds, amount of delay [-0.5 to 0.5]
                     );

// this function will process the input sound based on the state passed
// the input and output buffers should be the same size
void sf_reverb_process(sf_reverb_state_st *rv, double inputL, double inputR, double *outputL, double *outputR);
#endif // SNDFILTER_REVERB__H
