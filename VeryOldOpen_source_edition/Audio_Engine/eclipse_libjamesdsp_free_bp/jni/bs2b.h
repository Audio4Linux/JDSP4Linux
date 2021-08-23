#ifndef BS2B_H
#define BS2B_H
/* Minimum/maximum cut frequency (Hz) */
/* bs2b_set_level_fcut() */
#define BS2B_MINFCUT 300
#define BS2B_MAXFCUT 2000
/* Minimum/maximum feed level (dB * 10 @ low frequencies) */
/* bs2b_set_level_feed() */
#define BS2B_MINFEED 10   /* 1 dB */
#define BS2B_MAXFEED 150  /* 15 dB */
/* Default crossfeed levels */
/* Sets a new coefficients by new crossfeed value.
 * level = ( ( uint32_t )fcut | ( ( uint32_t )feed << 16 ) )
 * where 'feed' is crossfeeding level at low frequencies (dB * 10)
 * and 'fcut' is cut frecuency (Hz)
 */
#define BS2B_DEFAULT_CLEVEL  ((unsigned int)700 | ((unsigned int)45 << 16))
#define BS2B_CMOY_CLEVEL     ((unsigned int)700 | ((unsigned int)60 << 16))
#define BS2B_JMEIER_CLEVEL   ((unsigned int)650 | ((unsigned int)95 << 16))
typedef struct str_t_bs2bd
{
	double a0_lo, b1_lo;         /* Lowpass IIR filter coefficients */
	double a0_hi, a1_hi, b1_hi;  /* Highboost IIR filter coefficients */
	double gain;                 /* Global gain against overloading */
	/* Buffer of last filtered sample: [0] 1-st channel, [1] 2-d channel */
	struct { double asis[ 2 ], lo[ 2 ], hi[ 2 ]; } lfs;
} t_bs2bdp;
/* Get flevel value that used in function BS2BInit */
int BS2BCalculateflevel(unsigned int fcut, unsigned int gain);
void BS2BInit(t_bs2bdp *bs2bdp, unsigned int samplerate, int flevel);
/* sample poits to double floats native endians */
void BS2BProcess(t_bs2bdp *bs2bdp, double *sampleL, double *sampleR);
#endif	/* BS2B_H */