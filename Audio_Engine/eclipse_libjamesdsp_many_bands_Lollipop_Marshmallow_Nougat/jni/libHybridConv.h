/***************************************************************************
 *   Copyright (C) 2009 by Christian Borss                                 *
 *   christian.borss@rub.de                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __LIBHYBRIDCONV_H__
#define __LIBHYBRIDCONV_H__

#include "fftw3.h"

typedef struct str_HConvSingle
{
    int step;			// processing step counter
    int maxstep;			// number of processing steps per audio frame
    int mixpos;			// current frame index
    int framelength;		// number of samples per audio frame
    int frameLenMulFloatSize; // framelength * sizeof(float)
    int *steptask;			// processing tasks per step
    float *dft_time;		// DFT buffer (time domain)
    fftwf_complex *dft_freq;	// DFT buffer (frequency domain)
    float *in_freq_real;		// input buffer (frequency domain)
    float *in_freq_imag;		// input buffer (frequency domain)
    int num_filterbuf;		// number of filter segments
    float **filterbuf_freq_real;	// filter segments (frequency domain)
    float **filterbuf_freq_imag;	// filter segments (frequency domain)
    int num_mixbuf;			// number of mixing segments
    float **mixbuf_freq_real;	// mixing segments (frequency domain)
    float **mixbuf_freq_imag;	// mixing segments (frequency domain)
    float *history_time;		// history buffer (time domain)
    fftwf_plan fft;			// FFT transformation plan
    fftwf_plan ifft;		// IFFT transformation plan
} HConvSingle;
int hcFFTWThreadInit();
void hcFFTWThreadClean();
void hcProcess(HConvSingle *filter, float *x, float *y);
void hcProcessAdd(HConvSingle *filter, float *x, float *y);
void hcInitSingle(HConvSingle *filter, float *h, int hlen, int flen, int steps, int fftOptimize, int fftwThreads);
void hcCloseSingle(HConvSingle *filter);

#endif // __LIBHYBRIDCONV_H__
