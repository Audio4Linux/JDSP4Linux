#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "../jdsp_header.h"
void ArbitraryResponseEqualizerConstructor(JamesDSPLib *jdsp)
{
	jdsp->arbMag.instance.filterLen = InitArbitraryEq(&jdsp->arbMag.instance.coeffGen, 0);
	FFTConvolver2x2Init(&jdsp->arbMag.instance.convState);
	FFTConvolver2x2Init(&jdsp->arbMag.conv);
	float *kDelta = (float*)malloc(jdsp->arbMag.instance.filterLen * sizeof(float));
	memset(kDelta, 0, jdsp->arbMag.instance.filterLen * sizeof(float));
	kDelta[0] = 1.0f;
	FFTConvolver2x2LoadImpulseResponse(&jdsp->arbMag.instance.convState, (unsigned int)jdsp->blockSize, kDelta, kDelta, jdsp->arbMag.instance.filterLen);
	FFTConvolver2x2LoadImpulseResponse(&jdsp->arbMag.conv, (unsigned int)jdsp->blockSize, kDelta, kDelta, jdsp->arbMag.instance.filterLen);
	free(kDelta);
}
void ArbitraryResponseEqualizerDestructor(JamesDSPLib *jdsp)
{
	EqNodesFree(&jdsp->arbMag.instance.coeffGen);
	FFTConvolver2x2Free(&jdsp->arbMag.instance.convState);
	FFTConvolver2x2Free(&jdsp->arbMag.conv);
}
void ArbitraryResponseEqualizerStringParser(JamesDSPLib *jdsp, char *stringEq)
{
	ArbitraryEqString2SortedNodes(&jdsp->arbMag.instance.coeffGen, stringEq);
	float *eqFil = jdsp->arbMag.instance.coeffGen.GetFilter(&jdsp->arbMag.instance.coeffGen, (float)jdsp->fs);
	FFTConvolver2x2RefreshImpulseResponse(&jdsp->arbMag.instance.convState, &jdsp->arbMag.conv, eqFil, eqFil, jdsp->arbMag.instance.filterLen);
}
void ArbitraryResponseEqualizerEnable(JamesDSPLib *jdsp, char enable)
{
	if (jdsp->arbMagForceRefresh)
	{
		float *eqFil = jdsp->arbMag.instance.coeffGen.GetFilter(&jdsp->arbMag.instance.coeffGen, (float)jdsp->fs);
		FFTConvolver2x2Free(&jdsp->arbMag.instance.convState);
		FFTConvolver2x2Free(&jdsp->arbMag.conv);
		FFTConvolver2x2LoadImpulseResponse(&jdsp->arbMag.instance.convState, (unsigned int)jdsp->blockSize, eqFil, eqFil, jdsp->arbMag.instance.filterLen);
		FFTConvolver2x2LoadImpulseResponse(&jdsp->arbMag.conv, (unsigned int)jdsp->blockSize, eqFil, eqFil, jdsp->arbMag.instance.filterLen);
		jdsp->arbMagForceRefresh = 0;
	}
	if (enable)
		jdsp->arbitraryMagEnabled = 1;
}
void ArbitraryResponseEqualizerDisable(JamesDSPLib *jdsp)
{
	jdsp->arbitraryMagEnabled = 0;
}
void ArbitraryResponseEqualizerProcess(JamesDSPLib *jdsp, size_t n)
{
	FFTConvolver2x2Process(&jdsp->arbMag.conv, jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], jdsp->tmpBuffer[0], jdsp->tmpBuffer[1], (unsigned int)n);
}