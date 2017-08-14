#include <math.h>
#include "Tube.h"

int activate(tubeFilter *tubefilter, double samplerate, double *circuitdata, int warmupDuration, int insane)
{
	Real Fs = samplerate;
	// Passive components
	tubefilter->ci = circuitdata[0];
	tubefilter->ck = circuitdata[1];
	tubefilter->co = circuitdata[2];
	tubefilter->ro = circuitdata[3];
	tubefilter->rp = circuitdata[4];
	tubefilter->rg = circuitdata[5];
	tubefilter->ri = circuitdata[6];
	tubefilter->rk = circuitdata[7];
	tubefilter->e = 250.0; // Hard code to 250V
						   // 12AX7 triode model RSD-1
	tubefilter->v.g = 2.242e-3;
	tubefilter->v.mu = 103.2;
	tubefilter->v.gamma = 1.26;
	tubefilter->v.c = 3.4;
	tubefilter->v.gg = 6.177e-4;
	tubefilter->v.e = 1.314;
	tubefilter->v.cg = 9.901;
	tubefilter->v.ig0 = 8.025e-8;
	// 12AX7 triode model EHX-1
	tubefilter->v.g2 = 1.371e-3;
	tubefilter->v.mu2 = 86.9;
	tubefilter->v.gamma2 = 1.349;
	tubefilter->v.c2 = 4.56;
	tubefilter->v.gg2 = 3.263e-4;
	tubefilter->v.e2 = 1.156;
	tubefilter->v.cg2 = 11.99;
	tubefilter->v.ig02 = 3.917e-8;

	updateRValues(&tubefilter->ckt, tubefilter->ci, tubefilter->ck, tubefilter->co, tubefilter->e, 0.0, tubefilter->rp, tubefilter->rg, tubefilter->ri, tubefilter->rk, tubefilter->ro, 10000.0, Fs, insane, tubefilter->v);
	warmup_tubes(&tubefilter->ckt, warmupDuration);
	if (samplerate > 192000 || samplerate < 8000)
		return 0;
	tubefilter->fConst0 = (float)samplerate;
	tubefilter->fConst1 = (2 * tubefilter->fConst0);
	tubefilter->fConst2 = powf(tubefilter->fConst1, 2);
	tubefilter->fConst3 = (3 * tubefilter->fConst1);
	tubefilter->fConst4 = (1.0691560000000003e-08f * tubefilter->fConst1);
	tubefilter->fConst5 = (3.2074680000000005e-08f * tubefilter->fConst1);
	tubefilter->fConst6 = (0.044206800000000004f * tubefilter->fConst0);
	for (int i = 0; i < 4; i++)
	{
		tubefilter->fRec0[i] = 0;
		tubefilter->fRec1[i] = 0;
		tubefilter->fRec2[i] = 0;
		tubefilter->fRec3[i] = 0;
		tubefilter->fRec4[i] = 0;
		tubefilter->fRec5[i] = 0;
		tubefilter->fRec6[i] = 0;
		tubefilter->fRec7[i] = 0;
		tubefilter->fRec8[i] = 0;
		tubefilter->fRec9[i] = 0;
		tubefilter->fRec10[i] = 0;
		tubefilter->fRec11[i] = 0;
		tubefilter->fRec12[i] = 0;
		tubefilter->fRec13[i] = 0;
		tubefilter->fRec14[i] = 0;
		tubefilter->fRec15[i] = 0;
		tubefilter->fRec16[i] = 0;
		tubefilter->fRec17[i] = 0;
		tubefilter->fRec18[i] = 0;
		tubefilter->fRec19[i] = 0;
		tubefilter->fRec20[i] = 0;
		tubefilter->fRec21[i] = 0;
		tubefilter->fRec22[i] = 0;
		tubefilter->fRec23[i] = 0;
		tubefilter->fRec24[i] = 0;
	}
	return 1;
}
int InitTube(tubeFilter *tubefilter, double *circuitparameters, double samplerate, float tubedrive, int toneStackEnable, float bass, float middle, float treble, float tonestack, int warmupDuration, int insane)
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
	double circuitdata[8] = { 0.0000001, 0.00001 , 0.00000001, 1000000.0, 100000.0, 20000.0, 1000000.0, 1000.0 };
	if (!circuitparameters)
		circuitparameters = circuitdata;
	tubefilter->overdrived1Gain = from_dB(tubedrive)*0.6f;
	if (!activate(tubefilter, samplerate, circuitparameters, warmupDuration, insane))
		return 0;
	double gainDb = 17.0;
	double A = pow(10, gainDb / 40);
	double w0 = 25.132741228718345 / samplerate;
	double cs = cos(w0);
	double sn = sin(w0);
	double AL = sn / 2 * sqrt((A + 1 / A) * 2.333333333333334 + 2);
	double sq = 2 * sqrt(A) * AL;
	double b0 = A*((A + 1) - (A - 1)*cs + sq);
	double b1 = 2 * A*((A - 1) - (A + 1)*cs);
	double b2 = A*((A + 1) - (A - 1)*cs - sq);
	double a0 = (A + 1) + (A - 1)*cs + sq;
	double a1 = -2 * ((A - 1) + (A + 1)*cs);
	double a2 = (A + 1) + (A - 1)*cs - sq;
	tubefilter->pa0 = a0;
	tubefilter->pa1 = a1 / a0;
	tubefilter->pa2 = a2 / a0;
	tubefilter->pb0 = b0 / a0;
	tubefilter->pb1 = b1 / a0;
	tubefilter->pb2 = b2 / a0;
	tubefilter->toneStackEnable = toneStackEnable;
	if (tubefilter->toneStackEnable)
	{
		tubefilter->overdrived2Gain = 75.0f * from_dB(30.f - tubedrive);
		tubefilter->toneStack0 = middle / 20.0f;
		tubefilter->toneStack1 = (1.3784375e-06f * tubefilter->toneStack0);
		tubefilter->toneStack2 = (float)exp((3.4f * (bass / 20. - 1)));
		tubefilter->toneStack3 = (8.396625e-06f + ((7.405375e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.3784375000000003e-05f * tubefilter->toneStack2) - 5.7371875e-06f) - tubefilter->toneStack1))));
		tubefilter->toneStack4 = ((1.3062500000000001e-09f * tubefilter->toneStack2) - (1.30625e-10f * tubefilter->toneStack0));
		tubefilter->toneStack5 = (4.468750000000001e-09f * tubefilter->toneStack2);
		tubefilter->toneStack6 = (4.46875e-10f + (tubefilter->toneStack5 + (tubefilter->toneStack0 * (tubefilter->toneStack4 - 3.1625e-10f))));
		tubefilter->toneStack7 = (tubefilter->fConst1 * tubefilter->toneStack6);
		tubefilter->toneStack8 = (0.00055f * tubefilter->toneStack0);
		tubefilter->toneStack9 = (0.0250625f * tubefilter->toneStack2);
		tubefilter->toneStack10 = (tubefilter->fConst1 * (0.01842875f + (tubefilter->toneStack9 + tubefilter->toneStack8)));
		tubefilter->toneStack11 = ((tubefilter->toneStack10 + (tubefilter->fConst2 * (tubefilter->toneStack7 - tubefilter->toneStack3))) - 1);
		tubefilter->toneStack12 = (tubefilter->fConst3 * tubefilter->toneStack6);
		tubefilter->toneStack13 = ((tubefilter->fConst2 * (tubefilter->toneStack3 + tubefilter->toneStack12)) - (3 + tubefilter->toneStack10));
		tubefilter->toneStack14 = ((tubefilter->toneStack10 + (tubefilter->fConst2 * (tubefilter->toneStack3 - tubefilter->toneStack12))) - 3);
		tubefilter->toneStack15 = (0 - (1 + (tubefilter->toneStack10 + (tubefilter->fConst2 * (tubefilter->toneStack3 + tubefilter->toneStack7)))));
		tubefilter->toneStack16 = (1.0f / tubefilter->toneStack15);
		tubefilter->toneStack17 = treble / 20.0f;
		tubefilter->toneStack18 = ((tubefilter->toneStack0 * (1.30625e-10f + tubefilter->toneStack4)) + (tubefilter->toneStack17 * ((4.46875e-10f - (4.46875e-10f * tubefilter->toneStack0)) + tubefilter->toneStack5)));
		tubefilter->toneStack19 = (tubefilter->fConst3 * tubefilter->toneStack18);
		tubefilter->toneStack20 = (2.55375e-07f + (((9.912500000000003e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (1.4128125e-06f - tubefilter->toneStack1)))
			+ (tubefilter->toneStack2 * (2.5537500000000007e-06f + (1.3784375000000003e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack21 = (6.25e-05f * tubefilter->toneStack17);
		tubefilter->toneStack22 = (tubefilter->toneStack8 + tubefilter->toneStack21);
		tubefilter->toneStack23 = (0.0025062500000000002f + (tubefilter->toneStack9 + tubefilter->toneStack22));
		tubefilter->toneStack24 = (tubefilter->fConst1 * tubefilter->toneStack23);
		tubefilter->toneStack25 = (tubefilter->toneStack24 + (tubefilter->fConst2 * (tubefilter->toneStack20 - tubefilter->toneStack19)));
		tubefilter->toneStack26 = (tubefilter->fConst1 * tubefilter->toneStack18);
		tubefilter->toneStack27 = (tubefilter->toneStack24 + (tubefilter->fConst2 * (tubefilter->toneStack26 - tubefilter->toneStack20)));
		tubefilter->toneStack28 = (tubefilter->fConst1 * (0 - tubefilter->toneStack23));
		tubefilter->toneStack29 = (tubefilter->toneStack28 + (tubefilter->fConst2 * (tubefilter->toneStack20 + tubefilter->toneStack19)));
		tubefilter->toneStack30 = (tubefilter->toneStack28 - (tubefilter->fConst2 * (tubefilter->toneStack20 + tubefilter->toneStack26)));
		tubefilter->toneStack31 = (int)tonestack;
		tubefilter->toneStack32 = ((tubefilter->toneStack31 == 23) / tubefilter->toneStack15);
		tubefilter->toneStack33 = (1.0607618000000002e-05f * tubefilter->toneStack0);
		tubefilter->toneStack34 = (3.1187760000000004e-05f + ((0.00032604000000000004f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((0.00011284700000000001f * tubefilter->toneStack2) - 1.9801382e-05f) - tubefilter->toneStack33))));
		tubefilter->toneStack35 = ((3.5814000000000013e-09f * tubefilter->toneStack2) - (3.3665160000000007e-10f * tubefilter->toneStack0));
		tubefilter->toneStack36 = (8.100000000000003e-09f * tubefilter->toneStack2);
		tubefilter->toneStack37 = (7.614000000000002e-10f + (tubefilter->toneStack36 + (tubefilter->toneStack0 * (tubefilter->toneStack35 - 4.247484000000001e-10f))));
		tubefilter->toneStack38 = (tubefilter->fConst1 * tubefilter->toneStack37);
		tubefilter->toneStack39 = (0.00188f * tubefilter->toneStack0);
		tubefilter->toneStack40 = (0.060025f * tubefilter->toneStack2);
		tubefilter->toneStack41 = (tubefilter->fConst1 * (0.027267350000000003f + (tubefilter->toneStack40 + tubefilter->toneStack39)));
		tubefilter->toneStack42 = ((tubefilter->toneStack41 + (tubefilter->fConst2 * (tubefilter->toneStack38 - tubefilter->toneStack34))) - 1);
		tubefilter->toneStack43 = (tubefilter->fConst3 * tubefilter->toneStack37);
		tubefilter->toneStack44 = ((tubefilter->fConst2 * (tubefilter->toneStack34 + tubefilter->toneStack43)) - (3 + tubefilter->toneStack41));
		tubefilter->toneStack45 = ((tubefilter->toneStack41 + (tubefilter->fConst2 * (tubefilter->toneStack34 - tubefilter->toneStack43))) - 3);
		tubefilter->toneStack46 = (0 - (1 + (tubefilter->toneStack41 + (tubefilter->fConst2 * (tubefilter->toneStack34 + tubefilter->toneStack38)))));
		tubefilter->toneStack47 = (1.0f / tubefilter->toneStack46);
		tubefilter->toneStack48 = ((tubefilter->toneStack0 * (3.3665160000000007e-10f + tubefilter->toneStack35)) + (tubefilter->toneStack17 * ((7.614000000000002e-10f - (7.614000000000002e-10f * tubefilter->toneStack0)) + tubefilter->toneStack36)));
		tubefilter->toneStack49 = (tubefilter->fConst3 * tubefilter->toneStack48);
		tubefilter->toneStack50 = (1.9176000000000002e-07f + (((5.400000000000001e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (1.0654618000000002e-05f - tubefilter->toneStack33)))
			+ (tubefilter->toneStack2 * (2.0400000000000004e-06f + (0.00011284700000000001f * tubefilter->toneStack0)))));
		tubefilter->toneStack51 = (2.5e-05f * tubefilter->toneStack17);
		tubefilter->toneStack52 = (0.005642350000000001f + (tubefilter->toneStack40 + (tubefilter->toneStack39 + tubefilter->toneStack51)));
		tubefilter->toneStack53 = (tubefilter->fConst1 * tubefilter->toneStack52);
		tubefilter->toneStack54 = (tubefilter->toneStack53 + (tubefilter->fConst2 * (tubefilter->toneStack50 - tubefilter->toneStack49)));
		tubefilter->toneStack55 = (tubefilter->fConst1 * tubefilter->toneStack48);
		tubefilter->toneStack56 = (tubefilter->toneStack53 + (tubefilter->fConst2 * (tubefilter->toneStack55 - tubefilter->toneStack50)));
		tubefilter->toneStack57 = (tubefilter->fConst1 * (0 - tubefilter->toneStack52));
		tubefilter->toneStack58 = (tubefilter->toneStack57 + (tubefilter->fConst2 * (tubefilter->toneStack50 + tubefilter->toneStack49)));
		tubefilter->toneStack59 = (tubefilter->toneStack57 - (tubefilter->fConst2 * (tubefilter->toneStack50 + tubefilter->toneStack55)));
		tubefilter->toneStack60 = ((tubefilter->toneStack31 == 24) / tubefilter->toneStack46);
		tubefilter->toneStack61 = (4.7117500000000004e-07f * tubefilter->toneStack0);
		tubefilter->toneStack62 = (0.00011998125000000002f * tubefilter->toneStack2);
		tubefilter->toneStack63 = (5.718000000000001e-06f + (tubefilter->toneStack62 + (tubefilter->toneStack0 * (((1.1779375000000001e-05f * tubefilter->toneStack2) - 4.199450000000001e-06f) - tubefilter->toneStack61))));
		tubefilter->toneStack64 = ((1.0281250000000001e-09f * tubefilter->toneStack2) - (4.1125e-11f * tubefilter->toneStack0));
		tubefilter->toneStack65 = (7.343750000000001e-09f * tubefilter->toneStack2);
		tubefilter->toneStack66 = (2.9375e-10f + (tubefilter->toneStack65 + (tubefilter->toneStack0 * (tubefilter->toneStack64 - 2.52625e-10f))));
		tubefilter->toneStack67 = (tubefilter->fConst1 * tubefilter->toneStack66);
		tubefilter->toneStack68 = (0.00047000000000000004f * tubefilter->toneStack0);
		tubefilter->toneStack69 = (tubefilter->fConst1 * (0.015765f + (tubefilter->toneStack9 + tubefilter->toneStack68)));
		tubefilter->toneStack70 = ((tubefilter->toneStack69 + (tubefilter->fConst2 * (tubefilter->toneStack67 - tubefilter->toneStack63))) - 1);
		tubefilter->toneStack71 = (tubefilter->fConst3 * tubefilter->toneStack66);
		tubefilter->toneStack72 = ((tubefilter->fConst2 * (tubefilter->toneStack63 + tubefilter->toneStack71)) - (3 + tubefilter->toneStack69));
		tubefilter->toneStack73 = ((tubefilter->toneStack69 + (tubefilter->fConst2 * (tubefilter->toneStack63 - tubefilter->toneStack71))) - 3);
		tubefilter->toneStack74 = (0 - (1 + (tubefilter->toneStack69 + (tubefilter->fConst2 * (tubefilter->toneStack63 + tubefilter->toneStack67)))));
		tubefilter->toneStack75 = (1.0f / tubefilter->toneStack74);
		tubefilter->toneStack76 = ((tubefilter->toneStack0 * (4.1125e-11f + tubefilter->toneStack64)) + (tubefilter->toneStack17 * ((2.9375e-10f - (2.9375e-10f * tubefilter->toneStack0)) + tubefilter->toneStack65)));
		tubefilter->toneStack77 = (tubefilter->fConst3 * tubefilter->toneStack76);
		tubefilter->toneStack78 = (9.187500000000001e-07f * tubefilter->toneStack17);
		tubefilter->toneStack79 = (9.925e-08f + ((tubefilter->toneStack78 + (tubefilter->toneStack0 * (5.0055e-07f - tubefilter->toneStack61))) + (tubefilter->toneStack2 * (2.48125e-06f + (1.1779375000000001e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack80 = (0.0010025f + (tubefilter->toneStack9 + (tubefilter->toneStack21 + tubefilter->toneStack68)));
		tubefilter->toneStack81 = (tubefilter->fConst1 * tubefilter->toneStack80);
		tubefilter->toneStack82 = (tubefilter->toneStack81 + (tubefilter->fConst2 * (tubefilter->toneStack79 - tubefilter->toneStack77)));
		tubefilter->toneStack83 = (tubefilter->fConst1 * tubefilter->toneStack76);
		tubefilter->toneStack84 = (tubefilter->toneStack81 + (tubefilter->fConst2 * (tubefilter->toneStack83 - tubefilter->toneStack79)));
		tubefilter->toneStack85 = (tubefilter->fConst1 * (0 - tubefilter->toneStack80));
		tubefilter->toneStack86 = (tubefilter->toneStack85 + (tubefilter->fConst2 * (tubefilter->toneStack79 + tubefilter->toneStack77)));
		tubefilter->toneStack87 = (tubefilter->toneStack85 - (tubefilter->fConst2 * (tubefilter->toneStack79 + tubefilter->toneStack83)));
		tubefilter->toneStack88 = ((tubefilter->toneStack31 == 22) / tubefilter->toneStack74);
		tubefilter->toneStack89 = (3.059375000000001e-07f * tubefilter->toneStack0);
		tubefilter->toneStack90 = (1.5468750000000003e-06f + ((1.2718750000000003e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((3.0593750000000007e-06f * tubefilter->toneStack2) - 8.696875000000003e-07f) - tubefilter->toneStack89))));
		tubefilter->toneStack91 = ((2.646875e-10f * tubefilter->toneStack2) - (2.6468750000000002e-11f * tubefilter->toneStack0));
		tubefilter->toneStack92 = (7.5625e-10f * tubefilter->toneStack2);
		tubefilter->toneStack93 = (7.562500000000001e-11f + (tubefilter->toneStack92 + (tubefilter->toneStack0 * (tubefilter->toneStack91 - 4.915625000000001e-11f))));
		tubefilter->toneStack94 = (tubefilter->fConst1 * tubefilter->toneStack93);
		tubefilter->toneStack95 = (0.005562500000000001f * tubefilter->toneStack2);
		tubefilter->toneStack96 = (tubefilter->fConst1 * (0.005018750000000001f + (tubefilter->toneStack8 + tubefilter->toneStack95)));
		tubefilter->toneStack97 = ((tubefilter->toneStack96 + (tubefilter->fConst2 * (tubefilter->toneStack94 - tubefilter->toneStack90))) - 1);
		tubefilter->toneStack98 = (tubefilter->fConst3 * tubefilter->toneStack93);
		tubefilter->toneStack99 = ((tubefilter->fConst2 * (tubefilter->toneStack90 + tubefilter->toneStack98)) - (3 + tubefilter->toneStack96));
		tubefilter->toneStack100 = ((tubefilter->toneStack96 + (tubefilter->fConst2 * (tubefilter->toneStack90 - tubefilter->toneStack98))) - 3);
		tubefilter->toneStack101 = (0 - (1 + (tubefilter->toneStack96 + (tubefilter->fConst2 * (tubefilter->toneStack90 + tubefilter->toneStack94)))));
		tubefilter->toneStack102 = (1.0f / tubefilter->toneStack101);
		tubefilter->toneStack103 = ((tubefilter->toneStack0 * (2.6468750000000002e-11f + tubefilter->toneStack91)) + (tubefilter->toneStack17 * ((7.562500000000001e-11f - (7.562500000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack92)));
		tubefilter->toneStack104 = (tubefilter->fConst3 * tubefilter->toneStack103);
		tubefilter->toneStack105 = (6.1875e-08f + (((2.75e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (3.403125000000001e-07f - tubefilter->toneStack89))) + (tubefilter->toneStack2 * (6.1875e-07f + (3.0593750000000007e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack106 = (0.00055625f + (tubefilter->toneStack22 + tubefilter->toneStack95));
		tubefilter->toneStack107 = (tubefilter->fConst1 * tubefilter->toneStack106);
		tubefilter->toneStack108 = (tubefilter->toneStack107 + (tubefilter->fConst2 * (tubefilter->toneStack105 - tubefilter->toneStack104)));
		tubefilter->toneStack109 = (tubefilter->fConst1 * tubefilter->toneStack103);
		tubefilter->toneStack110 = (tubefilter->toneStack107 + (tubefilter->fConst2 * (tubefilter->toneStack109 - tubefilter->toneStack105)));
		tubefilter->toneStack111 = (tubefilter->fConst1 * (0 - tubefilter->toneStack106));
		tubefilter->toneStack112 = (tubefilter->toneStack111 + (tubefilter->fConst2 * (tubefilter->toneStack105 + tubefilter->toneStack104)));
		tubefilter->toneStack113 = (tubefilter->toneStack111 - (tubefilter->fConst2 * (tubefilter->toneStack105 + tubefilter->toneStack109)));
		tubefilter->toneStack114 = ((tubefilter->toneStack31 == 21) / tubefilter->toneStack101);
		tubefilter->toneStack115 = (2.2193400000000003e-07f * tubefilter->toneStack0);
		tubefilter->toneStack116 = (2.7073879999999998e-06f + ((4.9553415999999996e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((4.882548000000001e-06f * tubefilter->toneStack2) - 1.964318e-06f) - tubefilter->toneStack115))));
		tubefilter->toneStack117 = ((3.4212992000000004e-10f * tubefilter->toneStack2) - (1.5551360000000004e-11f * tubefilter->toneStack0));
		tubefilter->toneStack118 = (2.3521432000000003e-09f * tubefilter->toneStack2);
		tubefilter->toneStack119 = (1.0691560000000001e-10f + (tubefilter->toneStack118 + (tubefilter->toneStack0 * (tubefilter->toneStack117 - 9.136424e-11f))));
		tubefilter->toneStack120 = (tubefilter->fConst1 * tubefilter->toneStack119);
		tubefilter->toneStack121 = (0.0103884f * tubefilter->toneStack2);
		tubefilter->toneStack122 = (tubefilter->fConst1 * (0.009920600000000002f + (tubefilter->toneStack68 + tubefilter->toneStack121)));
		tubefilter->toneStack123 = ((tubefilter->toneStack122 + (tubefilter->fConst2 * (tubefilter->toneStack120 - tubefilter->toneStack116))) - 1);
		tubefilter->toneStack124 = (tubefilter->fConst3 * tubefilter->toneStack119);
		tubefilter->toneStack125 = ((tubefilter->fConst2 * (tubefilter->toneStack116 + tubefilter->toneStack124)) - (3 + tubefilter->toneStack122));
		tubefilter->toneStack126 = ((tubefilter->toneStack122 + (tubefilter->fConst2 * (tubefilter->toneStack116 - tubefilter->toneStack124))) - 3);
		tubefilter->toneStack127 = (0 - (1 + (tubefilter->toneStack122 + (tubefilter->fConst2 * (tubefilter->toneStack116 + tubefilter->toneStack120)))));
		tubefilter->toneStack128 = (1.0f / tubefilter->toneStack127);
		tubefilter->toneStack129 = ((tubefilter->toneStack0 * (1.5551360000000004e-11f + tubefilter->toneStack117)) + (tubefilter->toneStack17 * ((1.0691560000000001e-10f - (1.0691560000000001e-10f * tubefilter->toneStack0)) + tubefilter->toneStack118)));
		tubefilter->toneStack130 = (tubefilter->fConst3 * tubefilter->toneStack129);
		tubefilter->toneStack131 = (4.3428e-08f + (((4.5496e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (2.4468200000000005e-07f - tubefilter->toneStack115))) + (tubefilter->toneStack2 * (9.55416e-07f + (4.882548000000001e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack132 = (0.00047220000000000004f + (tubefilter->toneStack121 + (tubefilter->toneStack68 + (4.84e-05f * tubefilter->toneStack17))));
		tubefilter->toneStack133 = (tubefilter->fConst1 * tubefilter->toneStack132);
		tubefilter->toneStack134 = (tubefilter->toneStack133 + (tubefilter->fConst2 * (tubefilter->toneStack131 - tubefilter->toneStack130)));
		tubefilter->toneStack135 = (tubefilter->fConst1 * tubefilter->toneStack129);
		tubefilter->toneStack136 = (tubefilter->toneStack133 + (tubefilter->fConst2 * (tubefilter->toneStack135 - tubefilter->toneStack131)));
		tubefilter->toneStack137 = (tubefilter->fConst1 * (0 - tubefilter->toneStack132));
		tubefilter->toneStack138 = (tubefilter->toneStack137 + (tubefilter->fConst2 * (tubefilter->toneStack131 + tubefilter->toneStack130)));
		tubefilter->toneStack139 = (tubefilter->toneStack137 - (tubefilter->fConst2 * (tubefilter->toneStack131 + tubefilter->toneStack135)));
		tubefilter->toneStack140 = ((tubefilter->toneStack31 == 20) / tubefilter->toneStack127);
		tubefilter->toneStack141 = (2.3926056000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack142 = (1.0875480000000001e-05f * tubefilter->toneStack2);
		tubefilter->toneStack143 = (1.1144196800000003e-06f + ((3.659304000000001e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * ((tubefilter->toneStack142 - 4.347578400000001e-07f) - tubefilter->toneStack141))));
		tubefilter->toneStack144 = ((1.4413132800000006e-09f * tubefilter->toneStack2) - (3.1708892160000014e-11f * tubefilter->toneStack0));
		tubefilter->toneStack145 = (3.403100800000001e-09f * tubefilter->toneStack2);
		tubefilter->toneStack146 = (7.486821760000003e-11f + (tubefilter->toneStack145 + (tubefilter->toneStack0 * (tubefilter->toneStack144 - 4.315932544000001e-11f))));
		tubefilter->toneStack147 = (tubefilter->fConst1 * tubefilter->toneStack146);
		tubefilter->toneStack148 = (0.00048400000000000006f * tubefilter->toneStack0);
		tubefilter->toneStack149 = (0.022470000000000004f * tubefilter->toneStack2);
		tubefilter->toneStack150 = (tubefilter->toneStack149 + tubefilter->toneStack148);
		tubefilter->toneStack151 = (tubefilter->fConst1 * (0.00358974f + tubefilter->toneStack150));
		tubefilter->toneStack152 = ((tubefilter->toneStack151 + (tubefilter->fConst2 * (tubefilter->toneStack147 - tubefilter->toneStack143))) - 1);
		tubefilter->toneStack153 = (tubefilter->fConst3 * tubefilter->toneStack146);
		tubefilter->toneStack154 = ((tubefilter->fConst2 * (tubefilter->toneStack143 + tubefilter->toneStack153)) - (3 + tubefilter->toneStack151));
		tubefilter->toneStack155 = ((tubefilter->toneStack151 + (tubefilter->fConst2 * (tubefilter->toneStack143 - tubefilter->toneStack153))) - 3);
		tubefilter->toneStack156 = (0 - (1 + (tubefilter->toneStack151 + (tubefilter->fConst2 * (tubefilter->toneStack143 + tubefilter->toneStack147)))));
		tubefilter->toneStack157 = (1.0f / tubefilter->toneStack156);
		tubefilter->toneStack158 = ((tubefilter->toneStack0 * (3.1708892160000014e-11f + tubefilter->toneStack144)) + (tubefilter->toneStack17 * ((7.486821760000003e-11f - (7.486821760000003e-11f * tubefilter->toneStack0)) + tubefilter->toneStack145)));
		tubefilter->toneStack159 = (tubefilter->fConst3 * tubefilter->toneStack158);
		tubefilter->toneStack160 = (1.0875480000000001e-05f * tubefilter->toneStack0);
		tubefilter->toneStack161 = (tubefilter->toneStack0 * (2.893061600000001e-07f - tubefilter->toneStack141));
		tubefilter->toneStack162 = (8.098288000000002e-08f + (((3.0937280000000007e-07f * tubefilter->toneStack17) + tubefilter->toneStack161) + (tubefilter->toneStack2 * (3.6810400000000007e-06f + tubefilter->toneStack160))));
		tubefilter->toneStack163 = (0.00049434f + (tubefilter->toneStack149 + (tubefilter->toneStack148 + (0.0001034f * tubefilter->toneStack17))));
		tubefilter->toneStack164 = (tubefilter->fConst1 * tubefilter->toneStack163);
		tubefilter->toneStack165 = (tubefilter->toneStack164 + (tubefilter->fConst2 * (tubefilter->toneStack162 - tubefilter->toneStack159)));
		tubefilter->toneStack166 = (tubefilter->fConst1 * tubefilter->toneStack158);
		tubefilter->toneStack167 = (tubefilter->toneStack164 + (tubefilter->fConst2 * (tubefilter->toneStack166 - tubefilter->toneStack162)));
		tubefilter->toneStack168 = (tubefilter->fConst1 * (0 - tubefilter->toneStack163));
		tubefilter->toneStack169 = (tubefilter->toneStack168 + (tubefilter->fConst2 * (tubefilter->toneStack162 + tubefilter->toneStack159)));
		tubefilter->toneStack170 = (tubefilter->toneStack168 - (tubefilter->fConst2 * (tubefilter->toneStack162 + tubefilter->toneStack166)));
		tubefilter->toneStack171 = ((tubefilter->toneStack31 == 19) / tubefilter->toneStack156);
		tubefilter->toneStack172 = (7.790052600000002e-07f * tubefilter->toneStack0);
		tubefilter->toneStack173 = (1.4106061200000003e-06f + ((3.7475640000000014e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((2.3606220000000006e-05f * tubefilter->toneStack2) - 3.2220474e-07f) - tubefilter->toneStack172))));
		tubefilter->toneStack174 = ((1.5406083e-09f * tubefilter->toneStack2) - (5.08400739e-11f * tubefilter->toneStack0));
		tubefilter->toneStack175 = (1.9775250000000004e-09f * tubefilter->toneStack2);
		tubefilter->toneStack176 = (6.5258325e-11f + (tubefilter->toneStack175 + (tubefilter->toneStack0 * (tubefilter->toneStack174 - 1.4418251099999996e-11f))));
		tubefilter->toneStack177 = (tubefilter->fConst1 * tubefilter->toneStack176);
		tubefilter->toneStack178 = (0.001551f * tubefilter->toneStack0);
		tubefilter->toneStack179 = (0.015220000000000001f * tubefilter->toneStack2);
		tubefilter->toneStack180 = (tubefilter->fConst1 * (0.0037192600000000003f + (tubefilter->toneStack179 + tubefilter->toneStack178)));
		tubefilter->toneStack181 = ((tubefilter->toneStack180 + (tubefilter->fConst2 * (tubefilter->toneStack177 - tubefilter->toneStack173))) - 1);
		tubefilter->toneStack182 = (tubefilter->fConst3 * tubefilter->toneStack176);
		tubefilter->toneStack183 = ((tubefilter->fConst2 * (tubefilter->toneStack173 + tubefilter->toneStack182)) - (3 + tubefilter->toneStack180));
		tubefilter->toneStack184 = ((tubefilter->toneStack180 + (tubefilter->fConst2 * (tubefilter->toneStack173 - tubefilter->toneStack182))) - 3);
		tubefilter->toneStack185 = (0 - (1 + (tubefilter->toneStack180 + (tubefilter->fConst2 * (tubefilter->toneStack173 + tubefilter->toneStack177)))));
		tubefilter->toneStack186 = (1.0f / tubefilter->toneStack185);
		tubefilter->toneStack187 = ((tubefilter->toneStack0 * (5.08400739e-11f + tubefilter->toneStack174)) + (tubefilter->toneStack17 * ((6.5258325e-11f - (6.5258325e-11f * tubefilter->toneStack0)) + tubefilter->toneStack175)));
		tubefilter->toneStack188 = (tubefilter->fConst3 * tubefilter->toneStack187);
		tubefilter->toneStack189 = (5.018112e-08f + (((1.7391e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (8.643102600000002e-07f - tubefilter->toneStack172)))
			+ (tubefilter->toneStack2 * (1.5206400000000001e-06f + (2.3606220000000006e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack190 = (0.0005022600000000001f + (tubefilter->toneStack179 + (tubefilter->toneStack178 + (5.4999999999999995e-05f * tubefilter->toneStack17))));
		tubefilter->toneStack191 = (tubefilter->fConst1 * tubefilter->toneStack190);
		tubefilter->toneStack192 = (tubefilter->toneStack191 + (tubefilter->fConst2 * (tubefilter->toneStack189 - tubefilter->toneStack188)));
		tubefilter->toneStack193 = (tubefilter->fConst1 * tubefilter->toneStack187);
		tubefilter->toneStack194 = (tubefilter->toneStack191 + (tubefilter->fConst2 * (tubefilter->toneStack193 - tubefilter->toneStack189)));
		tubefilter->toneStack195 = (tubefilter->fConst1 * (0 - tubefilter->toneStack190));
		tubefilter->toneStack196 = (tubefilter->toneStack195 + (tubefilter->fConst2 * (tubefilter->toneStack189 + tubefilter->toneStack188)));
		tubefilter->toneStack197 = (tubefilter->toneStack195 - (tubefilter->fConst2 * (tubefilter->toneStack189 + tubefilter->toneStack193)));
		tubefilter->toneStack198 = ((tubefilter->toneStack31 == 18) / tubefilter->toneStack185);
		tubefilter->toneStack199 = (4.7047000000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack200 = (5.107200000000001e-06f + ((0.00011849250000000002f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.1761750000000001e-05f * tubefilter->toneStack2) - 4.217780000000001e-06f) - tubefilter->toneStack199))));
		tubefilter->toneStack201 = ((4.1125e-10f * tubefilter->toneStack2) - (1.645e-11f * tubefilter->toneStack0));
		tubefilter->toneStack202 = (2.9375000000000002e-09f * tubefilter->toneStack2);
		tubefilter->toneStack203 = (1.175e-10f + (tubefilter->toneStack202 + (tubefilter->toneStack0 * (tubefilter->toneStack201 - 1.0105e-10f))));
		tubefilter->toneStack204 = (tubefilter->fConst1 * tubefilter->toneStack203);
		tubefilter->toneStack205 = (0.025025000000000002f * tubefilter->toneStack2);
		tubefilter->toneStack206 = (tubefilter->fConst1 * (0.015726f + (tubefilter->toneStack68 + tubefilter->toneStack205)));
		tubefilter->toneStack207 = ((tubefilter->toneStack206 + (tubefilter->fConst2 * (tubefilter->toneStack204 - tubefilter->toneStack200))) - 1);
		tubefilter->toneStack208 = (tubefilter->fConst3 * tubefilter->toneStack203);
		tubefilter->toneStack209 = ((tubefilter->fConst2 * (tubefilter->toneStack200 + tubefilter->toneStack208)) - (3 + tubefilter->toneStack206));
		tubefilter->toneStack210 = ((tubefilter->toneStack206 + (tubefilter->fConst2 * (tubefilter->toneStack200 - tubefilter->toneStack208))) - 3);
		tubefilter->toneStack211 = (0 - (1 + (tubefilter->toneStack206 + (tubefilter->fConst2 * (tubefilter->toneStack200 + tubefilter->toneStack204)))));
		tubefilter->toneStack212 = (1.0f / tubefilter->toneStack211);
		tubefilter->toneStack213 = ((tubefilter->toneStack0 * (1.645e-11f + tubefilter->toneStack201)) + (tubefilter->toneStack17 * ((1.175e-10f - (1.175e-10f * tubefilter->toneStack0)) + tubefilter->toneStack202)));
		tubefilter->toneStack214 = (tubefilter->fConst3 * tubefilter->toneStack213);
		tubefilter->toneStack215 = (3.9700000000000005e-08f + (((3.675000000000001e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (4.8222e-07f - tubefilter->toneStack199)))
			+ (tubefilter->toneStack2 * (9.925e-07f + (1.1761750000000001e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack216 = (0.001001f + (tubefilter->toneStack205 + (tubefilter->toneStack51 + tubefilter->toneStack68)));
		tubefilter->toneStack217 = (tubefilter->fConst1 * tubefilter->toneStack216);
		tubefilter->toneStack218 = (tubefilter->toneStack217 + (tubefilter->fConst2 * (tubefilter->toneStack215 - tubefilter->toneStack214)));
		tubefilter->toneStack219 = (tubefilter->fConst1 * tubefilter->toneStack213);
		tubefilter->toneStack220 = (tubefilter->toneStack217 + (tubefilter->fConst2 * (tubefilter->toneStack219 - tubefilter->toneStack215)));
		tubefilter->toneStack221 = (tubefilter->fConst1 * (0 - tubefilter->toneStack216));
		tubefilter->toneStack222 = (tubefilter->toneStack221 + (tubefilter->fConst2 * (tubefilter->toneStack215 + tubefilter->toneStack214)));
		tubefilter->toneStack223 = (tubefilter->toneStack221 - (tubefilter->fConst2 * (tubefilter->toneStack215 + tubefilter->toneStack219)));
		tubefilter->toneStack224 = ((tubefilter->toneStack31 == 17) / tubefilter->toneStack211);
		tubefilter->toneStack225 = (3.0896250000000005e-07f * tubefilter->toneStack0);
		tubefilter->toneStack226 = (6.338090000000001e-07f + ((1.8734760000000003e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.2358500000000002e-05f * tubefilter->toneStack2) - 1.361249999999999e-08f) - tubefilter->toneStack225))));
		tubefilter->toneStack227 = ((1.6037340000000005e-09f * tubefilter->toneStack2) - (4.0093350000000015e-11f * tubefilter->toneStack0));
		tubefilter->toneStack228 = (1.8198400000000004e-09f * tubefilter->toneStack2);
		tubefilter->toneStack229 = (4.5496000000000015e-11f + (tubefilter->toneStack228 + (tubefilter->toneStack0 * (tubefilter->toneStack227 - 5.40265e-12f))));
		tubefilter->toneStack230 = (tubefilter->fConst1 * tubefilter->toneStack229);
		tubefilter->toneStack231 = (tubefilter->fConst1 * (0.00208725f + (tubefilter->toneStack8 + tubefilter->toneStack149)));
		tubefilter->toneStack232 = ((tubefilter->toneStack231 + (tubefilter->fConst2 * (tubefilter->toneStack230 - tubefilter->toneStack226))) - 1);
		tubefilter->toneStack233 = (tubefilter->fConst3 * tubefilter->toneStack229);
		tubefilter->toneStack234 = ((tubefilter->fConst2 * (tubefilter->toneStack226 + tubefilter->toneStack233)) - (3 + tubefilter->toneStack231));
		tubefilter->toneStack235 = ((tubefilter->toneStack231 + (tubefilter->fConst2 * (tubefilter->toneStack226 - tubefilter->toneStack233))) - 3);
		tubefilter->toneStack236 = (0 - (1 + (tubefilter->toneStack231 + (tubefilter->fConst2 * (tubefilter->toneStack226 + tubefilter->toneStack230)))));
		tubefilter->toneStack237 = (1.0f / tubefilter->toneStack236);
		tubefilter->toneStack238 = ((tubefilter->toneStack0 * (4.0093350000000015e-11f + tubefilter->toneStack227)) + (tubefilter->toneStack17 * ((4.5496000000000015e-11f - (4.5496000000000015e-11f * tubefilter->toneStack0)) + tubefilter->toneStack228)));
		tubefilter->toneStack239 = (tubefilter->fConst3 * tubefilter->toneStack238);
		tubefilter->toneStack240 = (8.1169e-08f + (((1.6544000000000003e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (3.735875000000001e-07f - tubefilter->toneStack225))) + (tubefilter->toneStack2 * (3.24676e-06f + (1.2358500000000002e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack241 = (0.00011750000000000001f * tubefilter->toneStack17);
		tubefilter->toneStack242 = (0.0005617500000000001f + (tubefilter->toneStack149 + (tubefilter->toneStack8 + tubefilter->toneStack241)));
		tubefilter->toneStack243 = (tubefilter->fConst1 * tubefilter->toneStack242);
		tubefilter->toneStack244 = (tubefilter->toneStack243 + (tubefilter->fConst2 * (tubefilter->toneStack240 - tubefilter->toneStack239)));
		tubefilter->toneStack245 = (tubefilter->fConst1 * tubefilter->toneStack238);
		tubefilter->toneStack246 = (tubefilter->toneStack243 + (tubefilter->fConst2 * (tubefilter->toneStack245 - tubefilter->toneStack240)));
		tubefilter->toneStack247 = (tubefilter->fConst1 * (0 - tubefilter->toneStack242));
		tubefilter->toneStack248 = (tubefilter->toneStack247 + (tubefilter->fConst2 * (tubefilter->toneStack240 + tubefilter->toneStack239)));
		tubefilter->toneStack249 = (tubefilter->toneStack247 - (tubefilter->fConst2 * (tubefilter->toneStack240 + tubefilter->toneStack245)));
		tubefilter->toneStack250 = ((tubefilter->toneStack31 == 16) / tubefilter->toneStack236);
		tubefilter->toneStack251 = (2.7256800000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack252 = (1.4234760000000002e-06f + ((2.851440000000001e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((6.8142000000000025e-06f * tubefilter->toneStack2) - 7.876920000000001e-07f) - tubefilter->toneStack251))));
		tubefilter->toneStack253 = ((4.724676000000001e-10f * tubefilter->toneStack2) - (1.8898704000000002e-11f * tubefilter->toneStack0));
		tubefilter->toneStack254 = (1.6641900000000002e-09f * tubefilter->toneStack2);
		tubefilter->toneStack255 = (6.656760000000001e-11f + (tubefilter->toneStack254 + (tubefilter->toneStack0 * (tubefilter->toneStack253 - 4.7668896000000004e-11f))));
		tubefilter->toneStack256 = (tubefilter->fConst1 * tubefilter->toneStack255);
		tubefilter->toneStack257 = (0.0008200000000000001f * tubefilter->toneStack0);
		tubefilter->toneStack258 = (0.00831f * tubefilter->toneStack2);
		tubefilter->toneStack259 = (tubefilter->fConst1 * (0.005107400000000001f + (tubefilter->toneStack258 + tubefilter->toneStack257)));
		tubefilter->toneStack260 = ((tubefilter->toneStack259 + (tubefilter->fConst2 * (tubefilter->toneStack256 - tubefilter->toneStack252))) - 1);
		tubefilter->toneStack261 = (tubefilter->fConst3 * tubefilter->toneStack255);
		tubefilter->toneStack262 = ((tubefilter->fConst2 * (tubefilter->toneStack252 + tubefilter->toneStack261)) - (3 + tubefilter->toneStack259));
		tubefilter->toneStack263 = ((tubefilter->toneStack259 + (tubefilter->fConst2 * (tubefilter->toneStack252 - tubefilter->toneStack261))) - 3);
		tubefilter->toneStack264 = (0 - (1 + (tubefilter->toneStack259 + (tubefilter->fConst2 * (tubefilter->toneStack252 + tubefilter->toneStack256)))));
		tubefilter->toneStack265 = (1.0f / tubefilter->toneStack264);
		tubefilter->toneStack266 = ((tubefilter->toneStack0 * (1.8898704000000002e-11f + tubefilter->toneStack253)) + (tubefilter->toneStack17 * ((6.656760000000001e-11f - (6.656760000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack254)));
		tubefilter->toneStack267 = (tubefilter->fConst3 * tubefilter->toneStack266);
		tubefilter->toneStack268 = (3.1116000000000005e-08f + (((2.829e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (3.2176800000000005e-07f - tubefilter->toneStack251))) + (tubefilter->toneStack2 * (7.779000000000002e-07f + (6.8142000000000025e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack269 = (0.00033240000000000006f + (tubefilter->toneStack258 + (tubefilter->toneStack257 + (6e-05f * tubefilter->toneStack17))));
		tubefilter->toneStack270 = (tubefilter->fConst1 * tubefilter->toneStack269);
		tubefilter->toneStack271 = (tubefilter->toneStack270 + (tubefilter->fConst2 * (tubefilter->toneStack268 - tubefilter->toneStack267)));
		tubefilter->toneStack272 = (tubefilter->fConst1 * tubefilter->toneStack266);
		tubefilter->toneStack273 = (tubefilter->toneStack270 + (tubefilter->fConst2 * (tubefilter->toneStack272 - tubefilter->toneStack268)));
		tubefilter->toneStack274 = (tubefilter->fConst1 * (0 - tubefilter->toneStack269));
		tubefilter->toneStack275 = (tubefilter->toneStack274 + (tubefilter->fConst2 * (tubefilter->toneStack268 + tubefilter->toneStack267)));
		tubefilter->toneStack276 = (tubefilter->toneStack274 - (tubefilter->fConst2 * (tubefilter->toneStack268 + tubefilter->toneStack272)));
		tubefilter->toneStack277 = ((tubefilter->toneStack31 == 15) / tubefilter->toneStack264);
		tubefilter->toneStack278 = (4.0108000000000004e-07f * tubefilter->toneStack0);
		tubefilter->toneStack279 = (5.050300000000001e-06f + ((0.00010263250000000001f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.0027e-05f * tubefilter->toneStack2) - 3.5719200000000006e-06f) - tubefilter->toneStack278))));
		tubefilter->toneStack280 = ((9.45e-10f * tubefilter->toneStack2) - (3.78e-11f * tubefilter->toneStack0));
		tubefilter->toneStack281 = (6.75e-09f * tubefilter->toneStack2);
		tubefilter->toneStack282 = (2.7e-10f + (tubefilter->toneStack281 + (tubefilter->toneStack0 * (tubefilter->toneStack280 - 2.3219999999999998e-10f))));
		tubefilter->toneStack283 = (tubefilter->fConst1 * tubefilter->toneStack282);
		tubefilter->toneStack284 = (0.0004f * tubefilter->toneStack0);
		tubefilter->toneStack285 = (0.025067500000000003f * tubefilter->toneStack2);
		tubefilter->toneStack286 = (tubefilter->fConst1 * (0.0150702f + (tubefilter->toneStack285 + tubefilter->toneStack284)));
		tubefilter->toneStack287 = ((tubefilter->toneStack286 + (tubefilter->fConst2 * (tubefilter->toneStack283 - tubefilter->toneStack279))) - 1);
		tubefilter->toneStack288 = (tubefilter->fConst3 * tubefilter->toneStack282);
		tubefilter->toneStack289 = ((tubefilter->fConst2 * (tubefilter->toneStack279 + tubefilter->toneStack288)) - (3 + tubefilter->toneStack286));
		tubefilter->toneStack290 = ((tubefilter->toneStack286 + (tubefilter->fConst2 * (tubefilter->toneStack279 - tubefilter->toneStack288))) - 3);
		tubefilter->toneStack291 = (0 - (1 + (tubefilter->toneStack286 + (tubefilter->fConst2 * (tubefilter->toneStack279 + tubefilter->toneStack283)))));
		tubefilter->toneStack292 = (1.0f / tubefilter->toneStack291);
		tubefilter->toneStack293 = ((tubefilter->toneStack0 * (3.78e-11f + tubefilter->toneStack280)) + (tubefilter->toneStack17 * ((2.7e-10f - (2.7e-10f * tubefilter->toneStack0)) + tubefilter->toneStack281)));
		tubefilter->toneStack294 = (tubefilter->fConst3 * tubefilter->toneStack293);
		tubefilter->toneStack295 = (1.0530000000000001e-07f + (((9.45e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (4.2808000000000006e-07f - tubefilter->toneStack278))) + (tubefilter->toneStack2 * (2.6324999999999998e-06f + (1.0027e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack296 = (6.75e-05f * tubefilter->toneStack17);
		tubefilter->toneStack297 = (0.0010027f + (tubefilter->toneStack285 + (tubefilter->toneStack284 + tubefilter->toneStack296)));
		tubefilter->toneStack298 = (tubefilter->fConst1 * tubefilter->toneStack297);
		tubefilter->toneStack299 = (tubefilter->toneStack298 + (tubefilter->fConst2 * (tubefilter->toneStack295 - tubefilter->toneStack294)));
		tubefilter->toneStack300 = (tubefilter->fConst1 * tubefilter->toneStack293);
		tubefilter->toneStack301 = (tubefilter->toneStack298 + (tubefilter->fConst2 * (tubefilter->toneStack300 - tubefilter->toneStack295)));
		tubefilter->toneStack302 = (tubefilter->fConst1 * (0 - tubefilter->toneStack297));
		tubefilter->toneStack303 = (tubefilter->toneStack302 + (tubefilter->fConst2 * (tubefilter->toneStack295 + tubefilter->toneStack294)));
		tubefilter->toneStack304 = (tubefilter->toneStack302 - (tubefilter->fConst2 * (tubefilter->toneStack295 + tubefilter->toneStack300)));
		tubefilter->toneStack305 = ((tubefilter->toneStack31 == 14) / tubefilter->toneStack291);
		tubefilter->toneStack306 = (1.95976e-07f * tubefilter->toneStack0);
		tubefilter->toneStack307 = (9.060568000000001e-07f + ((8.801210000000002e-06f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((2.4497000000000004e-06f * tubefilter->toneStack2) - 4.3256399999999996e-07f) - tubefilter->toneStack306))));
		tubefilter->toneStack308 = ((2.0778120000000008e-10f * tubefilter->toneStack2) - (1.6622496000000003e-11f * tubefilter->toneStack0));
		tubefilter->toneStack309 = (5.553900000000002e-10f * tubefilter->toneStack2);
		tubefilter->toneStack310 = (4.4431200000000016e-11f + (tubefilter->toneStack309 + (tubefilter->toneStack0 * (tubefilter->toneStack308 - 2.7808704000000013e-11f))));
		tubefilter->toneStack311 = (tubefilter->fConst1 * tubefilter->toneStack310);
		tubefilter->toneStack312 = (0.00044f * tubefilter->toneStack0);
		tubefilter->toneStack313 = (0.0055675f * tubefilter->toneStack2);
		tubefilter->toneStack314 = (tubefilter->fConst1 * (0.0035049f + (tubefilter->toneStack313 + tubefilter->toneStack312)));
		tubefilter->toneStack315 = ((tubefilter->toneStack314 + (tubefilter->fConst2 * (tubefilter->toneStack311 - tubefilter->toneStack307))) - 1);
		tubefilter->toneStack316 = (tubefilter->fConst3 * tubefilter->toneStack310);
		tubefilter->toneStack317 = ((tubefilter->fConst2 * (tubefilter->toneStack307 + tubefilter->toneStack316)) - (3 + tubefilter->toneStack314));
		tubefilter->toneStack318 = ((tubefilter->toneStack314 + (tubefilter->fConst2 * (tubefilter->toneStack307 - tubefilter->toneStack316))) - 3);
		tubefilter->toneStack319 = (0 - (1 + (tubefilter->toneStack314 + (tubefilter->fConst2 * (tubefilter->toneStack307 + tubefilter->toneStack311)))));
		tubefilter->toneStack320 = (1.0f / tubefilter->toneStack319);
		tubefilter->toneStack321 = ((tubefilter->toneStack0 * (1.6622496000000003e-11f + tubefilter->toneStack308)) + (tubefilter->toneStack17 * ((4.4431200000000016e-11f - (4.4431200000000016e-11f * tubefilter->toneStack0)) + tubefilter->toneStack309)));
		tubefilter->toneStack322 = (tubefilter->fConst3 * tubefilter->toneStack321);
		tubefilter->toneStack323 = (4.585680000000001e-08f + (((2.0196000000000004e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (2.2567600000000002e-07f - tubefilter->toneStack306))) + (tubefilter->toneStack2 * (5.732100000000001e-07f + (2.4497000000000004e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack324 = (0.00044540000000000004f + (tubefilter->toneStack313 + (tubefilter->toneStack296 + tubefilter->toneStack312)));
		tubefilter->toneStack325 = (tubefilter->fConst1 * tubefilter->toneStack324);
		tubefilter->toneStack326 = (tubefilter->toneStack325 + (tubefilter->fConst2 * (tubefilter->toneStack323 - tubefilter->toneStack322)));
		tubefilter->toneStack327 = (tubefilter->fConst1 * tubefilter->toneStack321);
		tubefilter->toneStack328 = (tubefilter->toneStack325 + (tubefilter->fConst2 * (tubefilter->toneStack327 - tubefilter->toneStack323)));
		tubefilter->toneStack329 = (tubefilter->fConst1 * (0 - tubefilter->toneStack324));
		tubefilter->toneStack330 = (tubefilter->toneStack329 + (tubefilter->fConst2 * (tubefilter->toneStack323 + tubefilter->toneStack322)));
		tubefilter->toneStack331 = (tubefilter->toneStack329 - (tubefilter->fConst2 * (tubefilter->toneStack323 + tubefilter->toneStack327)));
		tubefilter->toneStack332 = ((tubefilter->toneStack31 == 13) / tubefilter->toneStack319);
		tubefilter->toneStack333 = (4.9434000000000004e-08f * tubefilter->toneStack0);
		tubefilter->toneStack334 = (7.748796000000001e-07f + ((2.8889960000000004e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((4.943400000000001e-06f * tubefilter->toneStack2) - 1.2634599999999999e-07f) - tubefilter->toneStack333))));
		tubefilter->toneStack335 = ((1.2443156000000004e-09f * tubefilter->toneStack2) - (1.2443156000000002e-11f * tubefilter->toneStack0));
		tubefilter->toneStack336 = (5.345780000000001e-09f * tubefilter->toneStack2);
		tubefilter->toneStack337 = (5.345780000000001e-11f + (tubefilter->toneStack336 + (tubefilter->toneStack0 * (tubefilter->toneStack335 - 4.101464400000001e-11f))));
		tubefilter->toneStack338 = (tubefilter->fConst1 * tubefilter->toneStack337);
		tubefilter->toneStack339 = (0.00022f * tubefilter->toneStack0);
		tubefilter->toneStack340 = (tubefilter->fConst1 * (0.0025277f + (tubefilter->toneStack149 + tubefilter->toneStack339)));
		tubefilter->toneStack341 = ((tubefilter->toneStack340 + (tubefilter->fConst2 * (tubefilter->toneStack338 - tubefilter->toneStack334))) - 1);
		tubefilter->toneStack342 = (tubefilter->fConst3 * tubefilter->toneStack337);
		tubefilter->toneStack343 = ((tubefilter->fConst2 * (tubefilter->toneStack334 + tubefilter->toneStack342)) - (3 + tubefilter->toneStack340));
		tubefilter->toneStack344 = ((tubefilter->toneStack340 + (tubefilter->fConst2 * (tubefilter->toneStack334 - tubefilter->toneStack342))) - 3);
		tubefilter->toneStack345 = (0 - (1 + (tubefilter->toneStack340 + (tubefilter->fConst2 * (tubefilter->toneStack334 + tubefilter->toneStack338)))));
		tubefilter->toneStack346 = (1.0f / tubefilter->toneStack345);
		tubefilter->toneStack347 = ((tubefilter->toneStack0 * (1.2443156000000002e-11f + tubefilter->toneStack335)) + (tubefilter->toneStack17 * ((5.345780000000001e-11f - (5.345780000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack336)));
		tubefilter->toneStack348 = (tubefilter->fConst3 * tubefilter->toneStack347);
		tubefilter->toneStack349 = (6.141960000000001e-08f + (((4.859800000000001e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (1.0113400000000001e-07f - tubefilter->toneStack333))) + (tubefilter->toneStack2 * (6.141960000000001e-06f
			+ (4.943400000000001e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack350 = (0.00022470000000000001f + (tubefilter->toneStack149 + (tubefilter->toneStack339 + (0.00023500000000000002f * tubefilter->toneStack17))));
		tubefilter->toneStack351 = (tubefilter->fConst1 * tubefilter->toneStack350);
		tubefilter->toneStack352 = (tubefilter->toneStack351 + (tubefilter->fConst2 * (tubefilter->toneStack349 - tubefilter->toneStack348)));
		tubefilter->toneStack353 = (tubefilter->fConst1 * tubefilter->toneStack347);
		tubefilter->toneStack354 = (tubefilter->toneStack351 + (tubefilter->fConst2 * (tubefilter->toneStack353 - tubefilter->toneStack349)));
		tubefilter->toneStack355 = (tubefilter->fConst1 * (0 - tubefilter->toneStack350));
		tubefilter->toneStack356 = (tubefilter->toneStack355 + (tubefilter->fConst2 * (tubefilter->toneStack349 + tubefilter->toneStack348)));
		tubefilter->toneStack357 = (tubefilter->toneStack355 - (tubefilter->fConst2 * (tubefilter->toneStack349 + tubefilter->toneStack353)));
		tubefilter->toneStack358 = ((tubefilter->toneStack31 == 12) / tubefilter->toneStack345);
		tubefilter->toneStack359 = (2.5587500000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack360 = (7.717400000000001e-07f + ((2.2033600000000005e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.0235000000000001e-05f * tubefilter->toneStack2) - 1.5537499999999997e-07f) - tubefilter->toneStack359))));
		tubefilter->toneStack361 = ((1.3959000000000001e-09f * tubefilter->toneStack2) - (3.48975e-11f * tubefilter->toneStack0));
		tubefilter->toneStack362 = (2.2090000000000005e-09f * tubefilter->toneStack2);
		tubefilter->toneStack363 = (5.522500000000001e-11f + (tubefilter->toneStack362 + (tubefilter->toneStack0 * (tubefilter->toneStack361 - 2.0327500000000007e-11f))));
		tubefilter->toneStack364 = (tubefilter->fConst1 * tubefilter->toneStack363);
		tubefilter->toneStack365 = (0.0005f * tubefilter->toneStack0);
		tubefilter->toneStack366 = (0.020470000000000002f * tubefilter->toneStack2);
		tubefilter->toneStack367 = (tubefilter->fConst1 * (0.0025092499999999998f + (tubefilter->toneStack366 + tubefilter->toneStack365)));
		tubefilter->toneStack368 = ((tubefilter->toneStack367 + (tubefilter->fConst2 * (tubefilter->toneStack364 - tubefilter->toneStack360))) - 1);
		tubefilter->toneStack369 = (tubefilter->fConst3 * tubefilter->toneStack363);
		tubefilter->toneStack370 = ((tubefilter->fConst2 * (tubefilter->toneStack360 + tubefilter->toneStack369)) - (3 + tubefilter->toneStack367));
		tubefilter->toneStack371 = ((tubefilter->toneStack367 + (tubefilter->fConst2 * (tubefilter->toneStack360 - tubefilter->toneStack369))) - 3);
		tubefilter->toneStack372 = (0 - (1 + (tubefilter->toneStack367 + (tubefilter->fConst2 * (tubefilter->toneStack360 + tubefilter->toneStack364)))));
		tubefilter->toneStack373 = (1.0f / tubefilter->toneStack372);
		tubefilter->toneStack374 = ((tubefilter->toneStack0 * (3.48975e-11f + tubefilter->toneStack361)) + (tubefilter->toneStack17 * ((5.522500000000001e-11f - (5.522500000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack362)));
		tubefilter->toneStack375 = (tubefilter->fConst3 * tubefilter->toneStack374);
		tubefilter->toneStack376 = (8.084000000000001e-08f + (((2.2090000000000003e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (3.146250000000001e-07f - tubefilter->toneStack359)))
			+ (tubefilter->toneStack2 * (3.2336000000000007e-06f + (1.0235000000000001e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack377 = (0.00051175f + (tubefilter->toneStack366 + (tubefilter->toneStack241 + tubefilter->toneStack365)));
		tubefilter->toneStack378 = (tubefilter->fConst1 * tubefilter->toneStack377);
		tubefilter->toneStack379 = (tubefilter->toneStack378 + (tubefilter->fConst2 * (tubefilter->toneStack376 - tubefilter->toneStack375)));
		tubefilter->toneStack380 = (tubefilter->fConst1 * tubefilter->toneStack374);
		tubefilter->toneStack381 = (tubefilter->toneStack378 + (tubefilter->fConst2 * (tubefilter->toneStack380 - tubefilter->toneStack376)));
		tubefilter->toneStack382 = (tubefilter->fConst1 * (0 - tubefilter->toneStack377));
		tubefilter->toneStack383 = (tubefilter->toneStack382 + (tubefilter->fConst2 * (tubefilter->toneStack376 + tubefilter->toneStack375)));
		tubefilter->toneStack384 = (tubefilter->toneStack382 - (tubefilter->fConst2 * (tubefilter->toneStack376 + tubefilter->toneStack380)));
		tubefilter->toneStack385 = ((tubefilter->toneStack31 == 11) / tubefilter->toneStack372);
		tubefilter->toneStack386 = (0.00022854915600000004f * tubefilter->toneStack0);
		tubefilter->toneStack387 = (0.00010871476000000002f + ((0.00010719478000000002f * tubefilter->toneStack2) + (tubefilter->toneStack0 * ((0.00012621831200000002f + (0.00022854915600000004f * tubefilter->toneStack2)) - tubefilter->toneStack386))));
		tubefilter->toneStack388 = ((3.421299200000001e-08f * tubefilter->toneStack2) - (3.421299200000001e-08f * tubefilter->toneStack0));
		tubefilter->toneStack389 = (1.0f + (tubefilter->toneStack2 + (93531720.34763868f * (tubefilter->toneStack0 * (2.3521432000000005e-08f + tubefilter->toneStack388)))));
		tubefilter->toneStack390 = (tubefilter->fConst4 * tubefilter->toneStack389);
		tubefilter->toneStack391 = (tubefilter->fConst1 * (0.036906800000000003f + ((0.022103400000000002f * tubefilter->toneStack2) + (0.01034f * tubefilter->toneStack0))));
		tubefilter->toneStack392 = ((tubefilter->toneStack391 + (tubefilter->fConst2 * (tubefilter->toneStack390 - tubefilter->toneStack387))) - 1);
		tubefilter->toneStack393 = (tubefilter->fConst5 * tubefilter->toneStack389);
		tubefilter->toneStack394 = ((tubefilter->fConst2 * (tubefilter->toneStack387 + tubefilter->toneStack393)) - (3 + tubefilter->toneStack391));
		tubefilter->toneStack395 = ((tubefilter->toneStack391 + (tubefilter->fConst2 * (tubefilter->toneStack387 - tubefilter->toneStack393))) - 3);
		tubefilter->toneStack396 = ((tubefilter->fConst2 * (0 - (tubefilter->toneStack387 + tubefilter->toneStack390))) - (1 + tubefilter->toneStack391));
		tubefilter->toneStack397 = (1.0f / tubefilter->toneStack396);
		tubefilter->toneStack398 = ((tubefilter->toneStack0 * (3.421299200000001e-08f + tubefilter->toneStack388)) + (tubefilter->toneStack17 * ((1.0691560000000003e-08f - (1.0691560000000003e-08f * tubefilter->toneStack0)) + (1.0691560000000003e-08f * tubefilter->toneStack2))));
		tubefilter->toneStack399 = (tubefilter->fConst3 * tubefilter->toneStack398);
		tubefilter->toneStack400 = (3.7947800000000004e-06f + (((1.5199800000000001e-06f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (0.00022961831200000004f - tubefilter->toneStack386))) + (tubefilter->toneStack2 * (3.7947800000000004e-06f + tubefilter->toneStack386))));
		tubefilter->toneStack401 = (1.0f + (tubefilter->toneStack2 + ((0.0046780133373146215f * tubefilter->toneStack17) + (0.4678013337314621f * tubefilter->toneStack0))));
		tubefilter->toneStack402 = (tubefilter->fConst6 * tubefilter->toneStack401);
		tubefilter->toneStack403 = (tubefilter->toneStack402 + (tubefilter->fConst2 * (tubefilter->toneStack400 - tubefilter->toneStack399)));
		tubefilter->toneStack404 = (tubefilter->fConst1 * tubefilter->toneStack398);
		tubefilter->toneStack405 = (tubefilter->toneStack402 + (tubefilter->fConst2 * (tubefilter->toneStack404 - tubefilter->toneStack400)));
		tubefilter->toneStack406 = (tubefilter->fConst1 * (0 - (0.022103400000000002f * tubefilter->toneStack401)));
		tubefilter->toneStack407 = (tubefilter->toneStack406 + (tubefilter->fConst2 * (tubefilter->toneStack400 + tubefilter->toneStack399)));
		tubefilter->toneStack408 = (tubefilter->toneStack406 - (tubefilter->fConst2 * (tubefilter->toneStack400 + tubefilter->toneStack404)));
		tubefilter->toneStack409 = ((tubefilter->toneStack31 == 10) / tubefilter->toneStack396);
		tubefilter->toneStack410 = (4.851e-08f * tubefilter->toneStack0);
		tubefilter->toneStack411 = (7.172000000000001e-07f + ((4.972000000000001e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((4.8510000000000015e-06f * tubefilter->toneStack2) - 4.2449000000000006e-07f) - tubefilter->toneStack410))));
		tubefilter->toneStack412 = ((2.6620000000000007e-10f * tubefilter->toneStack2) - (2.662e-12f * tubefilter->toneStack0));
		tubefilter->toneStack413 = (2.4200000000000003e-09f * tubefilter->toneStack2);
		tubefilter->toneStack414 = (2.4200000000000004e-11f + (tubefilter->toneStack413 + (tubefilter->toneStack0 * (tubefilter->toneStack412 - 2.1538000000000003e-11f))));
		tubefilter->toneStack415 = (tubefilter->fConst1 * tubefilter->toneStack414);
		tubefilter->toneStack416 = (0.022050000000000004f * tubefilter->toneStack2);
		tubefilter->toneStack417 = (tubefilter->fConst1 * (0.0046705f + (tubefilter->toneStack339 + tubefilter->toneStack416)));
		tubefilter->toneStack418 = ((tubefilter->toneStack417 + (tubefilter->fConst2 * (tubefilter->toneStack415 - tubefilter->toneStack411))) - 1);
		tubefilter->toneStack419 = (tubefilter->fConst3 * tubefilter->toneStack414);
		tubefilter->toneStack420 = ((tubefilter->fConst2 * (tubefilter->toneStack411 + tubefilter->toneStack419)) - (3 + tubefilter->toneStack417));
		tubefilter->toneStack421 = ((tubefilter->toneStack417 + (tubefilter->fConst2 * (tubefilter->toneStack411 - tubefilter->toneStack419))) - 3);
		tubefilter->toneStack422 = (0 - (1 + (tubefilter->toneStack417 + (tubefilter->fConst2 * (tubefilter->toneStack411 + tubefilter->toneStack415)))));
		tubefilter->toneStack423 = (1.0f / tubefilter->toneStack422);
		tubefilter->toneStack424 = ((tubefilter->toneStack0 * (2.662e-12f + tubefilter->toneStack412)) + (tubefilter->toneStack17 * ((2.4200000000000004e-11f - (2.4200000000000004e-11f * tubefilter->toneStack0)) + tubefilter->toneStack413)));
		tubefilter->toneStack425 = (tubefilter->fConst3 * tubefilter->toneStack424);
		tubefilter->toneStack426 = (1.32e-08f + (((2.2000000000000004e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (5.951000000000001e-08f - tubefilter->toneStack410))) + (tubefilter->toneStack2 * (1.32e-06f + (4.8510000000000015e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack427 = (0.00022050000000000002f + (tubefilter->toneStack416 + (tubefilter->toneStack339 + (5e-05f * tubefilter->toneStack17))));
		tubefilter->toneStack428 = (tubefilter->fConst1 * tubefilter->toneStack427);
		tubefilter->toneStack429 = (tubefilter->toneStack428 + (tubefilter->fConst2 * (tubefilter->toneStack426 - tubefilter->toneStack425)));
		tubefilter->toneStack430 = (tubefilter->fConst1 * tubefilter->toneStack424);
		tubefilter->toneStack431 = (tubefilter->toneStack428 + (tubefilter->fConst2 * (tubefilter->toneStack430 - tubefilter->toneStack426)));
		tubefilter->toneStack432 = (tubefilter->fConst1 * (0 - tubefilter->toneStack427));
		tubefilter->toneStack433 = (tubefilter->toneStack432 + (tubefilter->fConst2 * (tubefilter->toneStack426 + tubefilter->toneStack425)));
		tubefilter->toneStack434 = (tubefilter->toneStack432 - (tubefilter->fConst2 * (tubefilter->toneStack426 + tubefilter->toneStack430)));
		tubefilter->toneStack435 = ((tubefilter->toneStack31 == 9) / tubefilter->toneStack422);
		tubefilter->toneStack436 = (1.38796875e-06f * tubefilter->toneStack0);
		tubefilter->toneStack437 = (3.5279375000000002e-06f + ((3.1989375e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.38796875e-05f * tubefilter->toneStack2) - 1.6311937500000001e-06f) - tubefilter->toneStack436))));
		tubefilter->toneStack438 = ((1.0561781250000004e-09f * tubefilter->toneStack2) - (1.0561781250000003e-10f * tubefilter->toneStack0));
		tubefilter->toneStack439 = (1.9328750000000005e-09f * tubefilter->toneStack2);
		tubefilter->toneStack440 = (1.9328750000000007e-10f + (tubefilter->toneStack439 + (tubefilter->toneStack0 * (tubefilter->toneStack438 - 8.766968750000004e-11f))));
		tubefilter->toneStack441 = (tubefilter->fConst1 * tubefilter->toneStack440);
		tubefilter->toneStack442 = (0.001175f * tubefilter->toneStack0);
		tubefilter->toneStack443 = (0.011812500000000002f * tubefilter->toneStack2);
		tubefilter->toneStack444 = (tubefilter->fConst1 * (0.0065077500000000005f + (tubefilter->toneStack443 + tubefilter->toneStack442)));
		tubefilter->toneStack445 = ((tubefilter->toneStack444 + (tubefilter->fConst2 * (tubefilter->toneStack441 - tubefilter->toneStack437))) - 1);
		tubefilter->toneStack446 = (tubefilter->fConst3 * tubefilter->toneStack440);
		tubefilter->toneStack447 = ((tubefilter->fConst2 * (tubefilter->toneStack437 + tubefilter->toneStack446)) - (3 + tubefilter->toneStack444));
		tubefilter->toneStack448 = ((tubefilter->toneStack444 + (tubefilter->fConst2 * (tubefilter->toneStack437 - tubefilter->toneStack446))) - 3);
		tubefilter->toneStack449 = (0 - (1 + (tubefilter->toneStack444 + (tubefilter->fConst2 * (tubefilter->toneStack437 + tubefilter->toneStack441)))));
		tubefilter->toneStack450 = (1.0f / tubefilter->toneStack449);
		tubefilter->toneStack451 = ((tubefilter->toneStack0 * (1.0561781250000003e-10f + tubefilter->toneStack438)) + (tubefilter->toneStack17 * ((1.9328750000000007e-10f - (1.9328750000000007e-10f * tubefilter->toneStack0)) + tubefilter->toneStack439)));
		tubefilter->toneStack452 = (tubefilter->fConst3 * tubefilter->toneStack451);
		tubefilter->toneStack453 = (1.0633750000000002e-07f + (((3.2900000000000005e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (1.4614062500000001e-06f - tubefilter->toneStack436)))
			+ (tubefilter->toneStack2 * (1.0633750000000002e-06f + (1.38796875e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack454 = (tubefilter->toneStack21 + tubefilter->toneStack442);
		tubefilter->toneStack455 = (0.00118125f + (tubefilter->toneStack443 + tubefilter->toneStack454));
		tubefilter->toneStack456 = (tubefilter->fConst1 * tubefilter->toneStack455);
		tubefilter->toneStack457 = (tubefilter->toneStack456 + (tubefilter->fConst2 * (tubefilter->toneStack453 - tubefilter->toneStack452)));
		tubefilter->toneStack458 = (tubefilter->fConst1 * tubefilter->toneStack451);
		tubefilter->toneStack459 = (tubefilter->toneStack456 + (tubefilter->fConst2 * (tubefilter->toneStack458 - tubefilter->toneStack453)));
		tubefilter->toneStack460 = (tubefilter->fConst1 * (0 - tubefilter->toneStack455));
		tubefilter->toneStack461 = (tubefilter->toneStack460 + (tubefilter->fConst2 * (tubefilter->toneStack453 + tubefilter->toneStack452)));
		tubefilter->toneStack462 = (tubefilter->toneStack460 - (tubefilter->fConst2 * (tubefilter->toneStack453 + tubefilter->toneStack458)));
		tubefilter->toneStack463 = ((tubefilter->toneStack31 == 8) / tubefilter->toneStack449);
		tubefilter->toneStack464 = (3.0937500000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack465 = (1.2375000000000003e-05f * tubefilter->toneStack2);
		tubefilter->toneStack466 = (6.677000000000001e-07f + ((1.9448000000000004e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * ((tubefilter->toneStack465 - 2.1175000000000003e-08f) - tubefilter->toneStack464))));
		tubefilter->toneStack467 = ((1.7121500000000001e-09f * tubefilter->toneStack2) - (4.2803750000000003e-11f * tubefilter->toneStack0));
		tubefilter->toneStack468 = (1.9965000000000003e-09f * tubefilter->toneStack2);
		tubefilter->toneStack469 = (4.991250000000001e-11f + (tubefilter->toneStack468 + (tubefilter->toneStack0 * (tubefilter->toneStack467 - 7.108750000000004e-12f))));
		tubefilter->toneStack470 = (tubefilter->fConst1 * tubefilter->toneStack469);
		tubefilter->toneStack471 = (0.022500000000000003f * tubefilter->toneStack2);
		tubefilter->toneStack472 = (tubefilter->toneStack8 + tubefilter->toneStack471);
		tubefilter->toneStack473 = (tubefilter->fConst1 * (0.0021395000000000003f + tubefilter->toneStack472));
		tubefilter->toneStack474 = ((tubefilter->toneStack473 + (tubefilter->fConst2 * (tubefilter->toneStack470 - tubefilter->toneStack466))) - 1);
		tubefilter->toneStack475 = (tubefilter->fConst3 * tubefilter->toneStack469);
		tubefilter->toneStack476 = ((tubefilter->fConst2 * (tubefilter->toneStack466 + tubefilter->toneStack475)) - (3 + tubefilter->toneStack473));
		tubefilter->toneStack477 = ((tubefilter->toneStack473 + (tubefilter->fConst2 * (tubefilter->toneStack466 - tubefilter->toneStack475))) - 3);
		tubefilter->toneStack478 = (0 - (1 + (tubefilter->toneStack473 + (tubefilter->fConst2 * (tubefilter->toneStack466 + tubefilter->toneStack470)))));
		tubefilter->toneStack479 = (1.0f / tubefilter->toneStack478);
		tubefilter->toneStack480 = ((tubefilter->toneStack0 * (4.2803750000000003e-11f + tubefilter->toneStack467)) + (tubefilter->toneStack17 * ((4.991250000000001e-11f - (4.991250000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack468)));
		tubefilter->toneStack481 = (tubefilter->fConst3 * tubefilter->toneStack480);
		tubefilter->toneStack482 = (1.2375000000000003e-05f * tubefilter->toneStack0);
		tubefilter->toneStack483 = (tubefilter->toneStack0 * (3.781250000000001e-07f - tubefilter->toneStack464));
		tubefilter->toneStack484 = (8.690000000000002e-08f + (((1.815e-07f * tubefilter->toneStack17) + tubefilter->toneStack483) + (tubefilter->toneStack2 * (3.4760000000000007e-06f + tubefilter->toneStack482))));
		tubefilter->toneStack485 = (0.0005625000000000001f + (tubefilter->toneStack471 + (tubefilter->toneStack8 + (0.000125f * tubefilter->toneStack17))));
		tubefilter->toneStack486 = (tubefilter->fConst1 * tubefilter->toneStack485);
		tubefilter->toneStack487 = (tubefilter->toneStack486 + (tubefilter->fConst2 * (tubefilter->toneStack484 - tubefilter->toneStack481)));
		tubefilter->toneStack488 = (tubefilter->fConst1 * tubefilter->toneStack480);
		tubefilter->toneStack489 = (tubefilter->toneStack486 + (tubefilter->fConst2 * (tubefilter->toneStack488 - tubefilter->toneStack484)));
		tubefilter->toneStack490 = (tubefilter->fConst1 * (0 - tubefilter->toneStack485));
		tubefilter->toneStack491 = (tubefilter->toneStack490 + (tubefilter->fConst2 * (tubefilter->toneStack484 + tubefilter->toneStack481)));
		tubefilter->toneStack492 = (tubefilter->toneStack490 - (tubefilter->fConst2 * (tubefilter->toneStack484 + tubefilter->toneStack488)));
		tubefilter->toneStack493 = ((tubefilter->toneStack31 == 7) / tubefilter->toneStack478);
		tubefilter->toneStack494 = (3.0621250000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack495 = (5.442360000000002e-07f + ((1.784904e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.2248500000000003e-05f * tubefilter->toneStack2) - 5.596250000000005e-08f) - tubefilter->toneStack494))));
		tubefilter->toneStack496 = ((9.245610000000004e-10f * tubefilter->toneStack2) - (2.3114025000000008e-11f * tubefilter->toneStack0));
		tubefilter->toneStack497 = (1.0781100000000005e-09f * tubefilter->toneStack2);
		tubefilter->toneStack498 = (2.695275000000001e-11f + (tubefilter->toneStack497 + (tubefilter->toneStack0 * (tubefilter->toneStack496 - 3.8387250000000005e-12f))));
		tubefilter->toneStack499 = (tubefilter->fConst1 * tubefilter->toneStack498);
		tubefilter->toneStack500 = (0.02227f * tubefilter->toneStack2);
		tubefilter->toneStack501 = (tubefilter->fConst1 * (0.00207625f + (tubefilter->toneStack8 + tubefilter->toneStack500)));
		tubefilter->toneStack502 = ((tubefilter->toneStack501 + (tubefilter->fConst2 * (tubefilter->toneStack499 - tubefilter->toneStack495))) - 1);
		tubefilter->toneStack503 = (tubefilter->fConst3 * tubefilter->toneStack498);
		tubefilter->toneStack504 = ((tubefilter->fConst2 * (tubefilter->toneStack495 + tubefilter->toneStack503)) - (3 + tubefilter->toneStack501));
		tubefilter->toneStack505 = ((tubefilter->toneStack501 + (tubefilter->fConst2 * (tubefilter->toneStack495 - tubefilter->toneStack503))) - 3);
		tubefilter->toneStack506 = (0 - (1 + (tubefilter->toneStack501 + (tubefilter->fConst2 * (tubefilter->toneStack495 + tubefilter->toneStack499)))));
		tubefilter->toneStack507 = (1.0f / tubefilter->toneStack506);
		tubefilter->toneStack508 = ((tubefilter->toneStack0 * (2.3114025000000008e-11f + tubefilter->toneStack496)) + (tubefilter->toneStack17 * ((2.695275000000001e-11f - (2.695275000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack497)));
		tubefilter->toneStack509 = (tubefilter->fConst3 * tubefilter->toneStack508);
		tubefilter->toneStack510 = (4.6926e-08f + (((9.801000000000002e-08f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (3.433375000000001e-07f - tubefilter->toneStack494))) + (tubefilter->toneStack2 * (1.8770400000000002e-06f + (1.2248500000000003e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack511 = (0.0005567500000000001f + (tubefilter->toneStack500 + (tubefilter->toneStack8 + tubefilter->toneStack296)));
		tubefilter->toneStack512 = (tubefilter->fConst1 * tubefilter->toneStack511);
		tubefilter->toneStack513 = (tubefilter->toneStack512 + (tubefilter->fConst2 * (tubefilter->toneStack510 - tubefilter->toneStack509)));
		tubefilter->toneStack514 = (tubefilter->fConst1 * tubefilter->toneStack508);
		tubefilter->toneStack515 = (tubefilter->toneStack512 + (tubefilter->fConst2 * (tubefilter->toneStack514 - tubefilter->toneStack510)));
		tubefilter->toneStack516 = (tubefilter->fConst1 * (0 - tubefilter->toneStack511));
		tubefilter->toneStack517 = (tubefilter->toneStack516 + (tubefilter->fConst2 * (tubefilter->toneStack510 + tubefilter->toneStack509)));
		tubefilter->toneStack518 = (tubefilter->toneStack516 - (tubefilter->fConst2 * (tubefilter->toneStack510 + tubefilter->toneStack514)));
		tubefilter->toneStack519 = ((tubefilter->toneStack31 == 6) / tubefilter->toneStack506);
		tubefilter->toneStack520 = (1.08515e-06f + ((3.108600000000001e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * ((tubefilter->toneStack465 - 2.99475e-07f) - tubefilter->toneStack464))));
		tubefilter->toneStack521 = ((1.8513000000000002e-09f * tubefilter->toneStack2) - (4.628250000000001e-11f * tubefilter->toneStack0));
		tubefilter->toneStack522 = (3.3880000000000003e-09f * tubefilter->toneStack2);
		tubefilter->toneStack523 = (8.470000000000002e-11f + (tubefilter->toneStack522 + (tubefilter->toneStack0 * (tubefilter->toneStack521 - 3.8417500000000006e-11f))));
		tubefilter->toneStack524 = (tubefilter->fConst1 * tubefilter->toneStack523);
		tubefilter->toneStack525 = (tubefilter->fConst1 * (tubefilter->toneStack472 + 0.0031515000000000002f));
		tubefilter->toneStack526 = ((tubefilter->toneStack525 + (tubefilter->fConst2 * (tubefilter->toneStack524 - tubefilter->toneStack520))) - 1);
		tubefilter->toneStack527 = (tubefilter->fConst3 * tubefilter->toneStack523);
		tubefilter->toneStack528 = ((tubefilter->fConst2 * (tubefilter->toneStack520 + tubefilter->toneStack527)) - (3 + tubefilter->toneStack525));
		tubefilter->toneStack529 = ((tubefilter->toneStack525 + (tubefilter->fConst2 * (tubefilter->toneStack520 - tubefilter->toneStack527))) - 3);
		tubefilter->toneStack530 = (0 - (1 + (tubefilter->toneStack525 + (tubefilter->fConst2 * (tubefilter->toneStack520 + tubefilter->toneStack524)))));
		tubefilter->toneStack531 = (1.0f / tubefilter->toneStack530);
		tubefilter->toneStack532 = ((tubefilter->toneStack0 * (4.628250000000001e-11f + tubefilter->toneStack521)) + (tubefilter->toneStack17 * ((8.470000000000002e-11f - (8.470000000000002e-11f * tubefilter->toneStack0)) + tubefilter->toneStack522)));
		tubefilter->toneStack533 = (tubefilter->fConst3 * tubefilter->toneStack532);
		tubefilter->toneStack534 = (9.955000000000001e-08f + ((tubefilter->toneStack483 + (3.08e-07f * tubefilter->toneStack17)) + (tubefilter->toneStack2 * (tubefilter->toneStack482 + 3.982e-06f))));
		tubefilter->toneStack535 = (tubefilter->toneStack486 + (tubefilter->fConst2 * (tubefilter->toneStack534 - tubefilter->toneStack533)));
		tubefilter->toneStack536 = (tubefilter->fConst1 * tubefilter->toneStack532);
		tubefilter->toneStack537 = (tubefilter->toneStack486 + (tubefilter->fConst2 * (tubefilter->toneStack536 - tubefilter->toneStack534)));
		tubefilter->toneStack538 = (tubefilter->toneStack490 + (tubefilter->fConst2 * (tubefilter->toneStack534 + tubefilter->toneStack533)));
		tubefilter->toneStack539 = (tubefilter->toneStack490 - (tubefilter->fConst2 * (tubefilter->toneStack534 + tubefilter->toneStack536)));
		tubefilter->toneStack540 = ((tubefilter->toneStack31 == 5) / tubefilter->toneStack530);
		tubefilter->toneStack541 = (5.665800800000001e-07f + ((1.892924e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * ((tubefilter->toneStack142 - 6.207784000000001e-08f) - tubefilter->toneStack141))));
		tubefilter->toneStack542 = ((1.2661536800000005e-09f * tubefilter->toneStack2) - (2.7855380960000008e-11f * tubefilter->toneStack0));
		tubefilter->toneStack543 = (1.6515048000000004e-09f * tubefilter->toneStack2);
		tubefilter->toneStack544 = (3.6333105600000014e-11f + (tubefilter->toneStack543 + (tubefilter->toneStack0 * (tubefilter->toneStack542 - 8.477724640000006e-12f))));
		tubefilter->toneStack545 = (tubefilter->fConst1 * tubefilter->toneStack544);
		tubefilter->toneStack546 = (tubefilter->fConst1 * (tubefilter->toneStack150 + 0.0020497400000000004f));
		tubefilter->toneStack547 = ((tubefilter->toneStack546 + (tubefilter->fConst2 * (tubefilter->toneStack545 - tubefilter->toneStack541))) - 1);
		tubefilter->toneStack548 = (tubefilter->fConst3 * tubefilter->toneStack544);
		tubefilter->toneStack549 = ((tubefilter->fConst2 * (tubefilter->toneStack541 + tubefilter->toneStack548)) - (3 + tubefilter->toneStack546));
		tubefilter->toneStack550 = ((tubefilter->toneStack546 + (tubefilter->fConst2 * (tubefilter->toneStack541 - tubefilter->toneStack548))) - 3);
		tubefilter->toneStack551 = (0 - (1 + (tubefilter->toneStack546 + (tubefilter->fConst2 * (tubefilter->toneStack541 + tubefilter->toneStack545)))));
		tubefilter->toneStack552 = (1.0f / tubefilter->toneStack551);
		tubefilter->toneStack553 = ((tubefilter->toneStack0 * (2.7855380960000008e-11f + tubefilter->toneStack542)) + (tubefilter->toneStack17 * ((3.6333105600000014e-11f - (3.6333105600000014e-11f * tubefilter->toneStack0)) + tubefilter->toneStack543)));
		tubefilter->toneStack554 = (tubefilter->fConst3 * tubefilter->toneStack553);
		tubefilter->toneStack555 = (6.505928000000001e-08f + ((tubefilter->toneStack161 + (1.5013680000000003e-07f * tubefilter->toneStack17)) + (tubefilter->toneStack2 * (tubefilter->toneStack160 + 2.95724e-06f))));
		tubefilter->toneStack556 = (tubefilter->toneStack164 + (tubefilter->fConst2 * (tubefilter->toneStack555 - tubefilter->toneStack554)));
		tubefilter->toneStack557 = (tubefilter->fConst1 * tubefilter->toneStack553);
		tubefilter->toneStack558 = (tubefilter->toneStack164 + (tubefilter->fConst2 * (tubefilter->toneStack557 - tubefilter->toneStack555)));
		tubefilter->toneStack559 = (tubefilter->toneStack168 + (tubefilter->fConst2 * (tubefilter->toneStack555 + tubefilter->toneStack554)));
		tubefilter->toneStack560 = (tubefilter->toneStack168 - (tubefilter->fConst2 * (tubefilter->toneStack555 + tubefilter->toneStack557)));
		tubefilter->toneStack561 = ((tubefilter->toneStack31 == 4) / tubefilter->toneStack551);
		tubefilter->toneStack562 = (1.0855872000000003e-07f * tubefilter->toneStack0);
		tubefilter->toneStack563 = (3.222390000000001e-06f + (tubefilter->toneStack62 + (tubefilter->toneStack0 * (((5.6541000000000015e-06f * tubefilter->toneStack2) - 2.1333412800000006e-06f) - tubefilter->toneStack562))));
		tubefilter->toneStack564 = (4.935e-10f * tubefilter->toneStack2);
		tubefilter->toneStack565 = (tubefilter->toneStack564 - (9.4752e-12f * tubefilter->toneStack0));
		tubefilter->toneStack566 = (1.41e-10f + (tubefilter->toneStack65 + (tubefilter->toneStack0 * (tubefilter->toneStack565 - 1.315248e-10f))));
		tubefilter->toneStack567 = (tubefilter->fConst1 * tubefilter->toneStack566);
		tubefilter->toneStack568 = (0.0002256f * tubefilter->toneStack0);
		tubefilter->toneStack569 = (tubefilter->fConst1 * (0.015243699999999999f + (tubefilter->toneStack9 + tubefilter->toneStack568)));
		tubefilter->toneStack570 = ((tubefilter->toneStack569 + (tubefilter->fConst2 * (tubefilter->toneStack567 - tubefilter->toneStack563))) - 1);
		tubefilter->toneStack571 = (tubefilter->fConst3 * tubefilter->toneStack566);
		tubefilter->toneStack572 = ((tubefilter->fConst2 * (tubefilter->toneStack563 + tubefilter->toneStack571)) - (3 + tubefilter->toneStack569));
		tubefilter->toneStack573 = ((tubefilter->toneStack569 + (tubefilter->fConst2 * (tubefilter->toneStack563 - tubefilter->toneStack571))) - 3);
		tubefilter->toneStack574 = (0 - (1 + (tubefilter->toneStack569 + (tubefilter->fConst2 * (tubefilter->toneStack563 + tubefilter->toneStack567)))));
		tubefilter->toneStack575 = (1.0f / tubefilter->toneStack574);
		tubefilter->toneStack576 = (1.41e-10f - (1.41e-10f * tubefilter->toneStack0));
		tubefilter->toneStack577 = ((tubefilter->toneStack0 * (9.4752e-12f + tubefilter->toneStack565)) + (tubefilter->toneStack17 * (tubefilter->toneStack65 + tubefilter->toneStack576)));
		tubefilter->toneStack578 = (tubefilter->fConst3 * tubefilter->toneStack577);
		tubefilter->toneStack579 = (4.764000000000001e-08f + ((tubefilter->toneStack78 + (tubefilter->toneStack0 * (1.2265872000000003e-07f - tubefilter->toneStack562)))
			+ (tubefilter->toneStack2 * (2.48125e-06f + (5.6541000000000015e-06f * tubefilter->toneStack0)))));
		tubefilter->toneStack580 = (0.00048120000000000004f + (tubefilter->toneStack9 + (tubefilter->toneStack21 + tubefilter->toneStack568)));
		tubefilter->toneStack581 = (tubefilter->fConst1 * tubefilter->toneStack580);
		tubefilter->toneStack582 = (tubefilter->toneStack581 + (tubefilter->fConst2 * (tubefilter->toneStack579 - tubefilter->toneStack578)));
		tubefilter->toneStack583 = (tubefilter->fConst1 * tubefilter->toneStack577);
		tubefilter->toneStack584 = (tubefilter->toneStack581 + (tubefilter->fConst2 * (tubefilter->toneStack583 - tubefilter->toneStack579)));
		tubefilter->toneStack585 = (tubefilter->fConst1 * (0 - tubefilter->toneStack580));
		tubefilter->toneStack586 = (tubefilter->toneStack585 + (tubefilter->fConst2 * (tubefilter->toneStack579 + tubefilter->toneStack578)));
		tubefilter->toneStack587 = (tubefilter->toneStack585 - (tubefilter->fConst2 * (tubefilter->toneStack579 + tubefilter->toneStack583)));
		tubefilter->toneStack588 = ((tubefilter->toneStack31 == 3) / tubefilter->toneStack574);
		tubefilter->toneStack589 = (4.7056400000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack590 = (5.188640000000001e-06f + ((0.00011869100000000002f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.1764100000000001e-05f * tubefilter->toneStack2) - 4.215336e-06f) - tubefilter->toneStack589))));
		tubefilter->toneStack591 = (tubefilter->toneStack564 - (1.974e-11f * tubefilter->toneStack0));
		tubefilter->toneStack592 = (3.525e-09f * tubefilter->toneStack2);
		tubefilter->toneStack593 = (1.41e-10f + (tubefilter->toneStack592 + (tubefilter->toneStack0 * (tubefilter->toneStack591 - 1.2126e-10f))));
		tubefilter->toneStack594 = (tubefilter->fConst1 * tubefilter->toneStack593);
		tubefilter->toneStack595 = (0.02503f * tubefilter->toneStack2);
		tubefilter->toneStack596 = (tubefilter->fConst1 * (0.0157312f + (tubefilter->toneStack68 + tubefilter->toneStack595)));
		tubefilter->toneStack597 = ((tubefilter->toneStack596 + (tubefilter->fConst2 * (tubefilter->toneStack594 - tubefilter->toneStack590))) - 1);
		tubefilter->toneStack598 = (tubefilter->fConst3 * tubefilter->toneStack593);
		tubefilter->toneStack599 = ((tubefilter->fConst2 * (tubefilter->toneStack590 + tubefilter->toneStack598)) - (3 + tubefilter->toneStack596));
		tubefilter->toneStack600 = ((tubefilter->toneStack596 + (tubefilter->fConst2 * (tubefilter->toneStack590 - tubefilter->toneStack598))) - 3);
		tubefilter->toneStack601 = (0 - (1 + (tubefilter->toneStack596 + (tubefilter->fConst2 * (tubefilter->toneStack590 + tubefilter->toneStack594)))));
		tubefilter->toneStack602 = (1.0f / tubefilter->toneStack601);
		tubefilter->toneStack603 = ((tubefilter->toneStack0 * (1.974e-11f + tubefilter->toneStack591)) + (tubefilter->toneStack17 * (tubefilter->toneStack576 + tubefilter->toneStack592)));
		tubefilter->toneStack604 = (tubefilter->fConst3 * tubefilter->toneStack603);
		tubefilter->toneStack605 = (4.764000000000001e-08f + (((4.410000000000001e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (4.846640000000001e-07f - tubefilter->toneStack589)))
			+ (tubefilter->toneStack2 * (1.1910000000000001e-06f + (1.1764100000000001e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack606 = (0.0010012f + (tubefilter->toneStack595 + (tubefilter->toneStack68 + (3e-05f * tubefilter->toneStack17))));
		tubefilter->toneStack607 = (tubefilter->fConst1 * tubefilter->toneStack606);
		tubefilter->toneStack608 = (tubefilter->toneStack607 + (tubefilter->fConst2 * (tubefilter->toneStack605 - tubefilter->toneStack604)));
		tubefilter->toneStack609 = (tubefilter->fConst1 * tubefilter->toneStack603);
		tubefilter->toneStack610 = (tubefilter->toneStack607 + (tubefilter->fConst2 * (tubefilter->toneStack609 - tubefilter->toneStack605)));
		tubefilter->toneStack611 = (tubefilter->fConst1 * (0 - tubefilter->toneStack606));
		tubefilter->toneStack612 = (tubefilter->toneStack611 + (tubefilter->fConst2 * (tubefilter->toneStack605 + tubefilter->toneStack604)));
		tubefilter->toneStack613 = (tubefilter->toneStack611 - (tubefilter->fConst2 * (tubefilter->toneStack605 + tubefilter->toneStack609)));
		tubefilter->toneStack614 = ((tubefilter->toneStack31 == 2) / tubefilter->toneStack601);
		tubefilter->toneStack615 = (2.9448437500000003e-06f * tubefilter->toneStack0);
		tubefilter->toneStack616 = (1.2916875000000002e-05f + (tubefilter->toneStack62 + (tubefilter->toneStack0 * (((2.9448437500000007e-05f * tubefilter->toneStack2) - 8.731718750000001e-06f) - tubefilter->toneStack615))));
		tubefilter->toneStack617 = ((2.5703125000000004e-09f * tubefilter->toneStack2) - (2.5703125000000003e-10f * tubefilter->toneStack0));
		tubefilter->toneStack618 = (7.343750000000001e-10f + (tubefilter->toneStack65 + (tubefilter->toneStack0 * (tubefilter->toneStack617 - 4.773437500000001e-10f))));
		tubefilter->toneStack619 = (tubefilter->fConst1 * tubefilter->toneStack618);
		tubefilter->toneStack620 = (tubefilter->fConst1 * (0.01726875f + (tubefilter->toneStack9 + tubefilter->toneStack442)));
		tubefilter->toneStack621 = ((tubefilter->toneStack620 + (tubefilter->fConst2 * (tubefilter->toneStack619 - tubefilter->toneStack616))) - 1);
		tubefilter->toneStack622 = (tubefilter->fConst3 * tubefilter->toneStack618);
		tubefilter->toneStack623 = ((tubefilter->fConst2 * (tubefilter->toneStack616 + tubefilter->toneStack622)) - (3 + tubefilter->toneStack620));
		tubefilter->toneStack624 = ((tubefilter->toneStack620 + (tubefilter->fConst2 * (tubefilter->toneStack616 - tubefilter->toneStack622))) - 3);
		tubefilter->toneStack625 = (0 - (1 + (tubefilter->toneStack620 + (tubefilter->fConst2 * (tubefilter->toneStack616 + tubefilter->toneStack619)))));
		tubefilter->toneStack626 = (1.0f / tubefilter->toneStack625);
		tubefilter->toneStack627 = ((tubefilter->toneStack0 * (2.5703125000000003e-10f + tubefilter->toneStack617)) + (tubefilter->toneStack17 * (tubefilter->toneStack65 + (7.343750000000001e-10f - (7.343750000000001e-10f * tubefilter->toneStack0)))));
		tubefilter->toneStack628 = (tubefilter->fConst3 * tubefilter->toneStack627);
		tubefilter->toneStack629 = (2.48125e-07f + ((tubefilter->toneStack78 + (tubefilter->toneStack0 * (3.0182812500000004e-06f - tubefilter->toneStack615))) + (tubefilter->toneStack2 * (2.48125e-06f + (2.9448437500000007e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack630 = (0.0025062500000000002f + (tubefilter->toneStack9 + tubefilter->toneStack454));
		tubefilter->toneStack631 = (tubefilter->fConst1 * tubefilter->toneStack630);
		tubefilter->toneStack632 = (tubefilter->toneStack631 + (tubefilter->fConst2 * (tubefilter->toneStack629 - tubefilter->toneStack628)));
		tubefilter->toneStack633 = (tubefilter->fConst1 * tubefilter->toneStack627);
		tubefilter->toneStack634 = (tubefilter->toneStack631 + (tubefilter->fConst2 * (tubefilter->toneStack633 - tubefilter->toneStack629)));
		tubefilter->toneStack635 = (tubefilter->fConst1 * (0 - tubefilter->toneStack630));
		tubefilter->toneStack636 = (tubefilter->toneStack635 + (tubefilter->fConst2 * (tubefilter->toneStack629 + tubefilter->toneStack628)));
		tubefilter->toneStack637 = (tubefilter->toneStack635 - (tubefilter->fConst2 * (tubefilter->toneStack629 + tubefilter->toneStack633)));
		tubefilter->toneStack638 = ((tubefilter->toneStack31 == 1) / tubefilter->toneStack625);
		tubefilter->toneStack639 = (2.5312500000000006e-07f * tubefilter->toneStack0);
		tubefilter->toneStack640 = (7.4525e-07f + ((2.4210000000000004e-05f * tubefilter->toneStack2) + (tubefilter->toneStack0 * (((1.0125e-05f * tubefilter->toneStack2) - 2.75625e-07f) - tubefilter->toneStack639))));
		tubefilter->toneStack641 = ((7.650000000000002e-10f * tubefilter->toneStack2) - (1.9125000000000002e-11f * tubefilter->toneStack0));
		tubefilter->toneStack642 = (1.4000000000000001e-09f * tubefilter->toneStack2);
		tubefilter->toneStack643 = (3.500000000000001e-11f + (tubefilter->toneStack642 + (tubefilter->toneStack0 * (tubefilter->toneStack641 - 1.5875000000000007e-11f))));
		tubefilter->toneStack644 = (tubefilter->fConst1 * tubefilter->toneStack643);
		tubefilter->toneStack645 = (0.02025f * tubefilter->toneStack2);
		tubefilter->toneStack646 = (tubefilter->fConst1 * (0.0028087500000000005f + (tubefilter->toneStack365 + tubefilter->toneStack645)));
		tubefilter->toneStack647 = ((tubefilter->toneStack646 + (tubefilter->fConst2 * (tubefilter->toneStack644 - tubefilter->toneStack640))) - 1);
		tubefilter->toneStack648 = (tubefilter->fConst3 * tubefilter->toneStack643);
		tubefilter->toneStack649 = ((tubefilter->fConst2 * (tubefilter->toneStack640 + tubefilter->toneStack648)) - (3 + tubefilter->toneStack646));
		tubefilter->toneStack650 = ((tubefilter->toneStack646 + (tubefilter->fConst2 * (tubefilter->toneStack640 - tubefilter->toneStack648))) - 3);
		tubefilter->toneStack651 = (0 - (1 + (tubefilter->toneStack646 + (tubefilter->fConst2 * (tubefilter->toneStack640 + tubefilter->toneStack644)))));
		tubefilter->toneStack652 = (1.0f / tubefilter->toneStack651);
		tubefilter->toneStack653 = ((tubefilter->toneStack0 * (1.9125000000000002e-11f + tubefilter->toneStack641)) + (tubefilter->toneStack17 * ((3.500000000000001e-11f - (3.500000000000001e-11f * tubefilter->toneStack0)) + tubefilter->toneStack642)));
		tubefilter->toneStack654 = (tubefilter->fConst3 * tubefilter->toneStack653);
		tubefilter->toneStack655 = (4.525e-08f + (((1.4e-07f * tubefilter->toneStack17) + (tubefilter->toneStack0 * (2.8437500000000003e-07f - tubefilter->toneStack639))) + (tubefilter->toneStack2 * (1.8100000000000002e-06f + (1.0125e-05f * tubefilter->toneStack0)))));
		tubefilter->toneStack656 = (0.00050625f + (tubefilter->toneStack645 + (tubefilter->toneStack21 + tubefilter->toneStack365)));
		tubefilter->toneStack657 = (tubefilter->fConst1 * tubefilter->toneStack656);
		tubefilter->toneStack658 = (tubefilter->toneStack657 + (tubefilter->fConst2 * (tubefilter->toneStack655 - tubefilter->toneStack654)));
		tubefilter->toneStack659 = (tubefilter->fConst1 * tubefilter->toneStack653);
		tubefilter->toneStack660 = (tubefilter->toneStack657 + (tubefilter->fConst2 * (tubefilter->toneStack659 - tubefilter->toneStack655)));
		tubefilter->toneStack661 = (tubefilter->fConst1 * (0 - tubefilter->toneStack656));
		tubefilter->toneStack662 = (tubefilter->toneStack661 + (tubefilter->fConst2 * (tubefilter->toneStack655 + tubefilter->toneStack654)));
		tubefilter->toneStack663 = (tubefilter->toneStack661 - (tubefilter->fConst2 * (tubefilter->toneStack655 + tubefilter->toneStack659)));
		tubefilter->toneStack664 = ((tubefilter->toneStack31 == 0) / tubefilter->toneStack651);
		double A = pow(10, 1.45);
		double w0 = 6.283185307179586 * 1.0 / samplerate;
		double cs = cos(w0);
		double sn = sin(w0);
		double AL = sn / 2 * sqrt((A + 1 / A) * (1 / 0.2 - 1) + 2);
		double sq = 2 * sqrt(A) * AL;
		double b0 = A*((A + 1) - (A - 1)*cs + sq);
		double b1 = 2 * A*((A - 1) - (A + 1)*cs);
		double b2 = A*((A + 1) - (A - 1)*cs - sq);
		double a0 = (A + 1) + (A - 1)*cs + sq;
		double a1 = -2 * ((A - 1) + (A + 1)*cs);
		double a2 = (A + 1) + (A - 1)*cs - sq;
		tubefilter->pa0 = a0;
		tubefilter->pa1 = a1 / a0;
		tubefilter->pa2 = a2 / a0;
		tubefilter->pb0 = b0 / a0;
		tubefilter->pb1 = b1 / a0;
		tubefilter->pb2 = b2 / a0;
	}
	else
	{
		double A = pow(10, 17.0 / 40);
		double w0 = 25.132741228718345 / samplerate;
		double cs = cos(w0);
		double sn = sin(w0);
		double AL = sn / 2 * sqrt((A + 1 / A) * 2.333333333333334 + 2);
		double sq = 2 * sqrt(A) * AL;
		double b0 = A*((A + 1) - (A - 1)*cs + sq);
		double b1 = 2 * A*((A - 1) - (A + 1)*cs);
		double b2 = A*((A + 1) - (A - 1)*cs - sq);
		double a0 = (A + 1) + (A - 1)*cs + sq;
		double a1 = -2 * ((A - 1) + (A + 1)*cs);
		double a2 = (A + 1) + (A - 1)*cs - sq;
		tubefilter->pa0 = a0;
		tubefilter->pa1 = a1 / a0;
		tubefilter->pa2 = a2 / a0;
		tubefilter->pb0 = b0 / a0;
		tubefilter->pb1 = b1 / a0;
		tubefilter->pb2 = b2 / a0;
		tubefilter->overdrived2Gain = 28.0f * from_dB(30.f - tubedrive);
	}
	tubefilter->v1 = tubefilter->v2 = 0;
	return 1;
}
void processTube(tubeFilter *tubefilter, float* inputs, float* outputs, unsigned frames)
{
	float tubeout;
	unsigned i, j, iMinus1;
	if (tubefilter->toneStackEnable)
	{
		for (i = 0; i < frames; ++i)
		{
			//Step 1: read input sample as voltage for the source
			double ViE = inputs[i] * tubefilter->overdrived1Gain;
			tubeout = advanc(&tubefilter->ckt, ViE) * tubefilter->overdrived2Gain;

			//Tone Stack sim
			tubefilter->fRec0[0] = (tubeout - (tubefilter->toneStack16 * (((tubefilter->toneStack14 * tubefilter->fRec0[2]) + (tubefilter->toneStack13 * tubefilter->fRec0[1])) + (tubefilter->toneStack11 * tubefilter->fRec0[3]))));
			tubefilter->fRec1[0] = (tubeout - (tubefilter->toneStack47 * (((tubefilter->toneStack45 * tubefilter->fRec1[2]) + (tubefilter->toneStack44 * tubefilter->fRec1[1])) + (tubefilter->toneStack42 * tubefilter->fRec1[3]))));
			tubefilter->fRec2[0] = (tubeout - (tubefilter->toneStack75 * (((tubefilter->toneStack73 * tubefilter->fRec2[2]) + (tubefilter->toneStack72 * tubefilter->fRec2[1])) + (tubefilter->toneStack70 * tubefilter->fRec2[3]))));
			tubefilter->fRec3[0] = (tubeout - (tubefilter->toneStack102 * (((tubefilter->toneStack100 * tubefilter->fRec3[2]) + (tubefilter->toneStack99 * tubefilter->fRec3[1])) + (tubefilter->toneStack97 * tubefilter->fRec3[3]))));
			tubefilter->fRec4[0] = (tubeout - (tubefilter->toneStack128 * (((tubefilter->toneStack126 * tubefilter->fRec4[2]) + (tubefilter->toneStack125 * tubefilter->fRec4[1])) + (tubefilter->toneStack123 * tubefilter->fRec4[3]))));
			tubefilter->fRec5[0] = (tubeout - (tubefilter->toneStack157 * (((tubefilter->toneStack155 * tubefilter->fRec5[2]) + (tubefilter->toneStack154 * tubefilter->fRec5[1])) + (tubefilter->toneStack152 * tubefilter->fRec5[3]))));
			tubefilter->fRec6[0] = (tubeout - (tubefilter->toneStack186 * (((tubefilter->toneStack184 * tubefilter->fRec6[2]) + (tubefilter->toneStack183 * tubefilter->fRec6[1])) + (tubefilter->toneStack181 * tubefilter->fRec6[3]))));
			tubefilter->fRec7[0] = (tubeout - (tubefilter->toneStack212 * (((tubefilter->toneStack210 * tubefilter->fRec7[2]) + (tubefilter->toneStack209 * tubefilter->fRec7[1])) + (tubefilter->toneStack207 * tubefilter->fRec7[3]))));
			tubefilter->fRec8[0] = (tubeout - (tubefilter->toneStack237 * (((tubefilter->toneStack235 * tubefilter->fRec8[2]) + (tubefilter->toneStack234 * tubefilter->fRec8[1])) + (tubefilter->toneStack232 * tubefilter->fRec8[3]))));
			tubefilter->fRec9[0] = (tubeout - (tubefilter->toneStack265 * (((tubefilter->toneStack263 * tubefilter->fRec9[2]) + (tubefilter->toneStack262 * tubefilter->fRec9[1])) + (tubefilter->toneStack260 * tubefilter->fRec9[3]))));
			tubefilter->fRec10[0] = (tubeout - (tubefilter->toneStack292 * (((tubefilter->toneStack290 * tubefilter->fRec10[2]) + (tubefilter->toneStack289 * tubefilter->fRec10[1])) + (tubefilter->toneStack287 * tubefilter->fRec10[3]))));
			tubefilter->fRec11[0] = (tubeout - (tubefilter->toneStack320 * (((tubefilter->toneStack318 * tubefilter->fRec11[2]) + (tubefilter->toneStack317 * tubefilter->fRec11[1])) + (tubefilter->toneStack315 * tubefilter->fRec11[3]))));
			tubefilter->fRec12[0] = (tubeout - (tubefilter->toneStack346 * (((tubefilter->toneStack344 * tubefilter->fRec12[2]) + (tubefilter->toneStack343 * tubefilter->fRec12[1])) + (tubefilter->toneStack341 * tubefilter->fRec12[3]))));
			tubefilter->fRec13[0] = (tubeout - (tubefilter->toneStack373 * (((tubefilter->toneStack371 * tubefilter->fRec13[2]) + (tubefilter->toneStack370 * tubefilter->fRec13[1])) + (tubefilter->toneStack368 * tubefilter->fRec13[3]))));
			tubefilter->fRec14[0] = (tubeout - (tubefilter->toneStack397 * (((tubefilter->toneStack395 * tubefilter->fRec14[2]) + (tubefilter->toneStack394 * tubefilter->fRec14[1])) + (tubefilter->toneStack392 * tubefilter->fRec14[3]))));
			tubefilter->fRec15[0] = (tubeout - (tubefilter->toneStack423 * (((tubefilter->toneStack421 * tubefilter->fRec15[2]) + (tubefilter->toneStack420 * tubefilter->fRec15[1])) + (tubefilter->toneStack418 * tubefilter->fRec15[3]))));
			tubefilter->fRec16[0] = (tubeout - (tubefilter->toneStack450 * (((tubefilter->toneStack448 * tubefilter->fRec16[2]) + (tubefilter->toneStack447 * tubefilter->fRec16[1])) + (tubefilter->toneStack445 * tubefilter->fRec16[3]))));
			tubefilter->fRec17[0] = (tubeout - (tubefilter->toneStack479 * (((tubefilter->toneStack477 * tubefilter->fRec17[2]) + (tubefilter->toneStack476 * tubefilter->fRec17[1])) + (tubefilter->toneStack474 * tubefilter->fRec17[3]))));
			tubefilter->fRec18[0] = (tubeout - (tubefilter->toneStack507 * (((tubefilter->toneStack505 * tubefilter->fRec18[2]) + (tubefilter->toneStack504 * tubefilter->fRec18[1])) + (tubefilter->toneStack502 * tubefilter->fRec18[3]))));
			tubefilter->fRec19[0] = (tubeout - (tubefilter->toneStack531 * (((tubefilter->toneStack529 * tubefilter->fRec19[2]) + (tubefilter->toneStack528 * tubefilter->fRec19[1])) + (tubefilter->toneStack526 * tubefilter->fRec19[3]))));
			tubefilter->fRec20[0] = (tubeout - (tubefilter->toneStack552 * (((tubefilter->toneStack550 * tubefilter->fRec20[2]) + (tubefilter->toneStack549 * tubefilter->fRec20[1])) + (tubefilter->toneStack547 * tubefilter->fRec20[3]))));
			tubefilter->fRec21[0] = (tubeout - (tubefilter->toneStack575 * (((tubefilter->toneStack573 * tubefilter->fRec21[2]) + (tubefilter->toneStack572 * tubefilter->fRec21[1])) + (tubefilter->toneStack570 * tubefilter->fRec21[3]))));
			tubefilter->fRec22[0] = (tubeout - (tubefilter->toneStack602 * (((tubefilter->toneStack600 * tubefilter->fRec22[2]) + (tubefilter->toneStack599 * tubefilter->fRec22[1])) + (tubefilter->toneStack597 * tubefilter->fRec22[3]))));
			tubefilter->fRec23[0] = (tubeout - (tubefilter->toneStack626 * (((tubefilter->toneStack624 * tubefilter->fRec23[2]) + (tubefilter->toneStack623 * tubefilter->fRec23[1])) + (tubefilter->toneStack621 * tubefilter->fRec23[3]))));
			tubefilter->fRec24[0] = (tubeout - (tubefilter->toneStack652 * (((tubefilter->toneStack650 * tubefilter->fRec24[2]) + (tubefilter->toneStack649 * tubefilter->fRec24[1])) + (tubefilter->toneStack647 * tubefilter->fRec24[3]))));
			ViE = ((tubefilter->toneStack664 * ((tubefilter->toneStack663 * tubefilter->fRec24[0]) + ((tubefilter->toneStack662 * tubefilter->fRec24[1]) + ((tubefilter->toneStack660 * tubefilter->fRec24[3])
				+ (tubefilter->toneStack658 * tubefilter->fRec24[2]))))) + ((tubefilter->toneStack638 * ((tubefilter->toneStack637 * tubefilter->fRec23[0]) + ((tubefilter->toneStack636 * tubefilter->fRec23[1])
					+ ((tubefilter->toneStack634 * tubefilter->fRec23[3]) + (tubefilter->toneStack632 * tubefilter->fRec23[2])))))
					+ ((tubefilter->toneStack614 * ((tubefilter->toneStack613 * tubefilter->fRec22[0]) + ((tubefilter->toneStack612 * tubefilter->fRec22[1])
						+ ((tubefilter->toneStack610 * tubefilter->fRec22[3]) + (tubefilter->toneStack608 * tubefilter->fRec22[2])))))
						+ ((tubefilter->toneStack588 * ((tubefilter->toneStack587 * tubefilter->fRec21[0]) + ((tubefilter->toneStack586 * tubefilter->fRec21[1])
							+ ((tubefilter->toneStack584 * tubefilter->fRec21[3]) + (tubefilter->toneStack582 * tubefilter->fRec21[2])))))
							+ ((tubefilter->toneStack561 * ((tubefilter->toneStack560 * tubefilter->fRec20[0]) + ((tubefilter->toneStack559 * tubefilter->fRec20[1])
								+ ((tubefilter->toneStack558 * tubefilter->fRec20[3]) + (tubefilter->toneStack556 * tubefilter->fRec20[2])))))
								+ ((tubefilter->toneStack540 * ((tubefilter->toneStack539 * tubefilter->fRec19[0]) + ((tubefilter->toneStack538 * tubefilter->fRec19[1])
									+ ((tubefilter->toneStack537 * tubefilter->fRec19[3]) + (tubefilter->toneStack535 * tubefilter->fRec19[2])))))
									+ ((tubefilter->toneStack519 * ((tubefilter->toneStack518 * tubefilter->fRec18[0]) + ((tubefilter->toneStack517 * tubefilter->fRec18[1])
										+ ((tubefilter->toneStack515 * tubefilter->fRec18[3]) + (tubefilter->toneStack513 * tubefilter->fRec18[2])))))
										+ ((tubefilter->toneStack493 * ((tubefilter->toneStack492 * tubefilter->fRec17[0]) + ((tubefilter->toneStack491 * tubefilter->fRec17[1])
											+ ((tubefilter->toneStack489 * tubefilter->fRec17[3]) + (tubefilter->toneStack487 * tubefilter->fRec17[2])))))
											+ ((tubefilter->toneStack463 * ((tubefilter->toneStack462 * tubefilter->fRec16[0]) + ((tubefilter->toneStack461 * tubefilter->fRec16[1])
												+ ((tubefilter->toneStack459 * tubefilter->fRec16[3]) + (tubefilter->toneStack457 * tubefilter->fRec16[2])))))
												+ ((tubefilter->toneStack435 * ((tubefilter->toneStack434 * tubefilter->fRec15[0]) + ((tubefilter->toneStack433 * tubefilter->fRec15[1])
													+ ((tubefilter->toneStack431 * tubefilter->fRec15[3]) + (tubefilter->toneStack429 * tubefilter->fRec15[2])))))
													+ ((tubefilter->toneStack409 * ((tubefilter->toneStack408 * tubefilter->fRec14[0]) + ((tubefilter->toneStack407 * tubefilter->fRec14[1])
														+ ((tubefilter->toneStack405 * tubefilter->fRec14[3]) + (tubefilter->toneStack403 * tubefilter->fRec14[2])))))
														+ ((tubefilter->toneStack385 * ((tubefilter->toneStack384 * tubefilter->fRec13[0]) + ((tubefilter->toneStack383 * tubefilter->fRec13[1])
															+ ((tubefilter->toneStack381 * tubefilter->fRec13[3]) + (tubefilter->toneStack379 * tubefilter->fRec13[2])))))
															+ ((tubefilter->toneStack358 * ((tubefilter->toneStack357 * tubefilter->fRec12[0]) + ((tubefilter->toneStack356 * tubefilter->fRec12[1])
																+ ((tubefilter->toneStack354 * tubefilter->fRec12[3]) + (tubefilter->toneStack352 * tubefilter->fRec12[2])))))
																+ ((tubefilter->toneStack332 * ((tubefilter->toneStack331 * tubefilter->fRec11[0]) + ((tubefilter->toneStack330 * tubefilter->fRec11[1])
																	+ ((tubefilter->toneStack328 * tubefilter->fRec11[3]) + (tubefilter->toneStack326 * tubefilter->fRec11[2])))))
																	+ ((tubefilter->toneStack305 * ((tubefilter->toneStack304 * tubefilter->fRec10[0]) + ((tubefilter->toneStack303 * tubefilter->fRec10[1])
																		+ ((tubefilter->toneStack301 * tubefilter->fRec10[3]) + (tubefilter->toneStack299 * tubefilter->fRec10[2])))))
																		+ ((tubefilter->toneStack277 * ((tubefilter->toneStack276 * tubefilter->fRec9[0]) + ((tubefilter->toneStack275 * tubefilter->fRec9[1])
																			+ ((tubefilter->toneStack273 * tubefilter->fRec9[3]) + (tubefilter->toneStack271 * tubefilter->fRec9[2])))))
																			+ ((tubefilter->toneStack250 * ((tubefilter->toneStack249 * tubefilter->fRec8[0]) + ((tubefilter->toneStack248 * tubefilter->fRec8[1])
																				+ ((tubefilter->toneStack246 * tubefilter->fRec8[3]) + (tubefilter->toneStack244 * tubefilter->fRec8[2])))))
																				+ ((tubefilter->toneStack224 * ((tubefilter->toneStack223 * tubefilter->fRec7[0]) + ((tubefilter->toneStack222 * tubefilter->fRec7[1])
																					+ ((tubefilter->toneStack220 * tubefilter->fRec7[3]) + (tubefilter->toneStack218 * tubefilter->fRec7[2])))))
																					+ ((tubefilter->toneStack198 * ((tubefilter->toneStack197 * tubefilter->fRec6[0]) + ((tubefilter->toneStack196 * tubefilter->fRec6[1])
																						+ ((tubefilter->toneStack194 * tubefilter->fRec6[3]) + (tubefilter->toneStack192 * tubefilter->fRec6[2])))))
																						+ ((tubefilter->toneStack171 * ((tubefilter->toneStack170 * tubefilter->fRec5[0]) + ((tubefilter->toneStack169 * tubefilter->fRec5[1])
																							+ ((tubefilter->toneStack167 * tubefilter->fRec5[3]) + (tubefilter->toneStack165 * tubefilter->fRec5[2])))))
																							+ ((tubefilter->toneStack140 * ((tubefilter->toneStack139 * tubefilter->fRec4[0]) + ((tubefilter->toneStack138 * tubefilter->fRec4[1])
																								+ ((tubefilter->toneStack136 * tubefilter->fRec4[3]) + (tubefilter->toneStack134 * tubefilter->fRec4[2])))))
																								+ ((tubefilter->toneStack114 * ((tubefilter->toneStack113 * tubefilter->fRec3[0]) + ((tubefilter->toneStack112 * tubefilter->fRec3[1])
																									+ ((tubefilter->toneStack110 * tubefilter->fRec3[3]) + (tubefilter->toneStack108 * tubefilter->fRec3[2])))))
																									+ ((tubefilter->toneStack88 * ((tubefilter->toneStack87 * tubefilter->fRec2[0]) + ((tubefilter->toneStack86 * tubefilter->fRec2[1])
																										+ ((tubefilter->toneStack84 * tubefilter->fRec2[3]) + (tubefilter->toneStack82 * tubefilter->fRec2[2])))))
																										+ ((tubefilter->toneStack60 * ((tubefilter->toneStack59 * tubefilter->fRec1[0]) + ((tubefilter->toneStack58 * tubefilter->fRec1[1])
																											+ ((tubefilter->toneStack56 * tubefilter->fRec1[3]) + (tubefilter->toneStack54 * tubefilter->fRec1[2])))))
																											+ (tubefilter->toneStack32 * ((tubefilter->toneStack30 * tubefilter->fRec0[0]) + ((tubefilter->toneStack29 * tubefilter->fRec0[1])
																												+ ((tubefilter->toneStack27 * tubefilter->fRec0[3]) + (tubefilter->toneStack25 * tubefilter->fRec0[2]))))))))))))))))))))))))))))) - tubefilter->pa1 * tubefilter->v1 - tubefilter->pa2*tubefilter->v2;
			outputs[i] = tubefilter->pb0 * ViE + tubefilter->pb1 * tubefilter->v1 + tubefilter->pb2*tubefilter->v2;
			tubefilter->v2 = tubefilter->v1;
			tubefilter->v1 = ViE;
			// post processing
			for (j = 3; j > 0; j--)
			{
				iMinus1 = j - 1;
				tubefilter->fRec24[j] = (tubefilter->fRec24[iMinus1]);
				tubefilter->fRec23[j] = (tubefilter->fRec23[iMinus1]);
				tubefilter->fRec22[j] = (tubefilter->fRec22[iMinus1]);
				tubefilter->fRec21[j] = (tubefilter->fRec21[iMinus1]);
				tubefilter->fRec20[j] = (tubefilter->fRec20[iMinus1]);
				tubefilter->fRec19[j] = (tubefilter->fRec19[iMinus1]);
				tubefilter->fRec18[j] = (tubefilter->fRec18[iMinus1]);
				tubefilter->fRec17[j] = (tubefilter->fRec17[iMinus1]);
				tubefilter->fRec16[j] = (tubefilter->fRec16[iMinus1]);
				tubefilter->fRec15[j] = (tubefilter->fRec15[iMinus1]);
				tubefilter->fRec14[j] = (tubefilter->fRec14[iMinus1]);
				tubefilter->fRec13[j] = (tubefilter->fRec13[iMinus1]);
				tubefilter->fRec12[j] = (tubefilter->fRec12[iMinus1]);
				tubefilter->fRec11[j] = (tubefilter->fRec11[iMinus1]);
				tubefilter->fRec10[j] = (tubefilter->fRec10[iMinus1]);
				tubefilter->fRec9[j] = (tubefilter->fRec9[iMinus1]);
				tubefilter->fRec8[j] = (tubefilter->fRec8[iMinus1]);
				tubefilter->fRec7[j] = (tubefilter->fRec7[iMinus1]);
				tubefilter->fRec6[j] = (tubefilter->fRec6[iMinus1]);
				tubefilter->fRec5[j] = (tubefilter->fRec5[iMinus1]);
				tubefilter->fRec4[j] = (tubefilter->fRec4[iMinus1]);
				tubefilter->fRec3[j] = (tubefilter->fRec3[iMinus1]);
				tubefilter->fRec2[j] = (tubefilter->fRec2[iMinus1]);
				tubefilter->fRec1[j] = (tubefilter->fRec1[iMinus1]);
				tubefilter->fRec0[j] = (tubefilter->fRec0[iMinus1]);
			}
		}
	}
	else
	{
		for (i = 0; i < frames; ++i)
		{
			double ViE = inputs[i] * tubefilter->overdrived1Gain;
			ViE = (advanc(&tubefilter->ckt, ViE) * tubefilter->overdrived2Gain) - tubefilter->pa1 * tubefilter->v1 - tubefilter->pa2*tubefilter->v2;
			outputs[i] = tubefilter->pb0 * ViE + tubefilter->pb1 * tubefilter->v1 + tubefilter->pb2*tubefilter->v2;
			tubefilter->v2 = tubefilter->v1;
			tubefilter->v1 = ViE;
		}
	}
}
