#include <math.h>
#include "Tube.h"
int activate(tubeFilter *tubefilter, Real samplerate, Real *circuitdata, int warmupDuration, int insane)
{
	// Passive components
	tubefilter->ci = circuitdata[0];
	tubefilter->ck = circuitdata[1];
	tubefilter->co = circuitdata[2];
	tubefilter->ro = circuitdata[3];
	tubefilter->rp = circuitdata[4];
	tubefilter->rg = circuitdata[5];
	tubefilter->ri = circuitdata[6];
	tubefilter->rk = circuitdata[7];
	tubefilter->e = 250.0; // Hard code to 250V 12AX7 triode model RSD-1
	tubefilter->v.g = 2.242e-3;
	tubefilter->v.mu = 103.2;
	tubefilter->v.gamma = 1.26;
	tubefilter->v.c = 3.4;
	tubefilter->v.gg = 7.177e-2;
	tubefilter->v.e = 1.314;
	tubefilter->v.cg = 9.901;
	tubefilter->v.ig0 = 8.025e-8;
	updateRValues(&tubefilter->ckt, tubefilter->ci, tubefilter->ck, tubefilter->co, tubefilter->e, 0.0, tubefilter->rp, tubefilter->rg, tubefilter->ri, tubefilter->rk, tubefilter->ro, 10000.0, samplerate, tubefilter->v);
	warmup_tubes(&tubefilter->ckt, warmupDuration);
	return 1;
}
int InitTube(tubeFilter *tubefilter, Real *circuitparameters, double samplerate, double tubedrive, int warmupDuration, int insane)
{
	/*
	tubefilter->ci = 0.0000001;	//100nF
	tubefilter->ck = 0.00001;		//10uF
	tubefilter->co = 0.00000001;	//10nF
	tubefilter->ro = 1000000.0;		//1Mohm
	tubefilter->rp = 100000.0;		//100kohm
	tubefilter->rg = 20000.0;		//20kohm
	tubefilter->ri = 1000000.0;		//1Mohm
	tubefilter->rk = 1000.0;		//1kohm
	tubefilter->e = 250.0;		//250V
	*/
	Real circuitdata[8] = { 0.0000001, 0.00001 , 0.00000001, 1000000.0, 100000.0, 20000.0, 1000000.0, 1000.0 };
	if (!circuitparameters)
		circuitparameters = circuitdata;
	tubefilter->overdrived1Gain = exp(tubedrive / 20.0*log(10.0))*0.6;
	if (!activate(tubefilter, samplerate, circuitparameters, warmupDuration, insane))
		return 0;
	tubefilter->overdrived2Gain = 36.0 * exp((30.0 - tubedrive) / 20.0*log(10.0));
	return 1;
}
void processTube(tubeFilter *tubefilter, double* inputs, double* outputs, unsigned int frames)
{
	for (unsigned int i = 0; i < frames; ++i)
	{
		//Read input sample as voltage for the source
		outputs[i] = -advanc(&tubefilter->ckt, inputs[i] * tubefilter->overdrived1Gain) * tubefilter->overdrived2Gain;
	}
}
