#include <math.h>
#include "Tube.h"

int activate(tubeFilter *tubefilter, float samplerate, double *circuitdata, int warmupDuration, int insane)
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
	updateRValues(&tubefilter->ckt, tubefilter->ci, tubefilter->ck, tubefilter->co, tubefilter->e, 0.0, tubefilter->rp, tubefilter->rg, tubefilter->ri, tubefilter->rk, tubefilter->ro, 10000.0, samplerate, insane, tubefilter->v);
	warmup_tubes(&tubefilter->ckt, warmupDuration);
	if (samplerate > 96000.0 || samplerate < 4000.0)
		return 0;
	tubefilter->fConst0 = samplerate;
	tubefilter->fConst1 = (2.0f * tubefilter->fConst0);
	tubefilter->fConst2 = powf(tubefilter->fConst1, 2.0f);
	tubefilter->fConst3 = (3.0f * tubefilter->fConst1);
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
int InitTube(tubeFilter *tubefilter, double *circuitparameters, float samplerate, float tubedrive, float bass, float middle, float treble, int tonestack, int warmupDuration, int insane)
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
	tubefilter->overdrived2Gain = 75.0f * from_dB(30.f - tubedrive);
	float toneStack0 = middle / 20.0f;
	float toneStack1 = (1.3784375e-06f * toneStack0);
	float toneStack2 = (float)exp((3.4f * (bass / 20. - 1)));
	float toneStack3 = (8.396625e-06f + ((7.405375e-05f * toneStack2) + (toneStack0 * (((1.3784375000000003e-05f * toneStack2) - 5.7371875e-06f) - toneStack1))));
	float toneStack4 = ((1.3062500000000001e-09f * toneStack2) - (1.30625e-10f * toneStack0));
	float toneStack5 = (4.468750000000001e-09f * toneStack2);
	float toneStack6 = (4.46875e-10f + (toneStack5 + (toneStack0 * (toneStack4 - 3.1625e-10f))));
	float toneStack7 = (tubefilter->fConst1 * toneStack6);
	float toneStack8 = (0.00055f * toneStack0);
	float toneStack9 = (0.0250625f * toneStack2);
	float toneStack10 = (tubefilter->fConst1 * (0.01842875f + (toneStack9 + toneStack8)));
	tubefilter->toneStack11 = ((toneStack10 + (tubefilter->fConst2 * (toneStack7 - toneStack3))) - 1);
	float toneStack12 = (tubefilter->fConst3 * toneStack6);
	tubefilter->toneStack13 = ((tubefilter->fConst2 * (toneStack3 + toneStack12)) - (3.0f + toneStack10));
	tubefilter->toneStack14 = ((toneStack10 + (tubefilter->fConst2 * (toneStack3 - toneStack12))) - 3);
	float toneStack15 = (0 - (1 + (toneStack10 + (tubefilter->fConst2 * (toneStack3 + toneStack7)))));
	tubefilter->toneStack16 = (1.0f / toneStack15);
	float toneStack17 = treble / 20.0f;
	float toneStack18 = ((toneStack0 * (1.30625e-10f + toneStack4)) + (toneStack17 * ((4.46875e-10f - (4.46875e-10f * toneStack0)) + toneStack5)));
	float toneStack19 = (tubefilter->fConst3 * toneStack18);
	float toneStack20 = (2.55375e-07f + (((9.912500000000003e-07f * toneStack17) + (toneStack0 * (1.4128125e-06f - toneStack1)))
		+ (toneStack2 * (2.5537500000000007e-06f + (1.3784375000000003e-05f * toneStack0)))));
	float toneStack21 = (6.25e-05f * toneStack17);
	float toneStack22 = (toneStack8 + toneStack21);
	float toneStack23 = (0.0025062500000000002f + (toneStack9 + toneStack22));
	float toneStack24 = (tubefilter->fConst1 * toneStack23);
	tubefilter->toneStack25 = (toneStack24 + (tubefilter->fConst2 * (toneStack20 - toneStack19)));
	float toneStack26 = (tubefilter->fConst1 * toneStack18);
	tubefilter->toneStack27 = (toneStack24 + (tubefilter->fConst2 * (toneStack26 - toneStack20)));
	float toneStack28 = (tubefilter->fConst1 * -toneStack23);
	tubefilter->toneStack29 = (toneStack28 + (tubefilter->fConst2 * (toneStack20 + toneStack19)));
	tubefilter->toneStack30 = (toneStack28 - (tubefilter->fConst2 * (toneStack20 + toneStack26)));
	int toneStack31 = tonestack;
	tubefilter->toneStack32 = ((toneStack31 == 23) / toneStack15);
	float toneStack33 = (1.0607618000000002e-05f * toneStack0);
	float toneStack34 = (3.1187760000000004e-05f + ((0.00032604000000000004f * toneStack2) + (toneStack0 * (((0.00011284700000000001f * toneStack2) - 1.9801382e-05f) - toneStack33))));
	float toneStack35 = ((3.5814000000000013e-09f * toneStack2) - (3.3665160000000007e-10f * toneStack0));
	float toneStack36 = (8.100000000000003e-09f * toneStack2);
	float toneStack37 = (7.614000000000002e-10f + (toneStack36 + (toneStack0 * (toneStack35 - 4.247484000000001e-10f))));
	float toneStack38 = (tubefilter->fConst1 * toneStack37);
	float toneStack39 = (0.00188f * toneStack0);
	float toneStack40 = (0.060025f * toneStack2);
	float toneStack41 = (tubefilter->fConst1 * (0.027267350000000003f + (toneStack40 + toneStack39)));
	tubefilter->toneStack42 = ((toneStack41 + (tubefilter->fConst2 * (toneStack38 - toneStack34))) - 1.0f);
	float toneStack43 = (tubefilter->fConst3 * toneStack37);
	tubefilter->toneStack44 = ((tubefilter->fConst2 * (toneStack34 + toneStack43)) - (3 + toneStack41));
	tubefilter->toneStack45 = ((toneStack41 + (tubefilter->fConst2 * (toneStack34 - toneStack43))) - 3.0f);
	float toneStack46 = (0 - (1 + (toneStack41 + (tubefilter->fConst2 * (toneStack34 + toneStack38)))));
	tubefilter->toneStack47 = (1.0f / toneStack46);
	float toneStack48 = ((toneStack0 * (3.3665160000000007e-10f + toneStack35)) + (toneStack17 * ((7.614000000000002e-10f - (7.614000000000002e-10f * toneStack0)) + toneStack36)));
	float toneStack49 = (tubefilter->fConst3 * toneStack48);
	float toneStack50 = (1.9176000000000002e-07f + (((5.400000000000001e-07f * toneStack17) + (toneStack0 * (1.0654618000000002e-05f - toneStack33)))
		+ (toneStack2 * (2.0400000000000004e-06f + (0.00011284700000000001f * toneStack0)))));
	float toneStack51 = (2.5e-05f * toneStack17);
	float toneStack52 = (0.005642350000000001f + (toneStack40 + (toneStack39 + toneStack51)));
	float toneStack53 = (tubefilter->fConst1 * toneStack52);
	tubefilter->toneStack54 = (toneStack53 + (tubefilter->fConst2 * (toneStack50 - toneStack49)));
	float toneStack55 = (tubefilter->fConst1 * toneStack48);
	tubefilter->toneStack56 = (toneStack53 + (tubefilter->fConst2 * (toneStack55 - toneStack50)));
	float toneStack57 = (tubefilter->fConst1 * -toneStack52);
	tubefilter->toneStack58 = (toneStack57 + (tubefilter->fConst2 * (toneStack50 + toneStack49)));
	tubefilter->toneStack59 = (toneStack57 - (tubefilter->fConst2 * (toneStack50 + toneStack55)));
	tubefilter->toneStack60 = ((toneStack31 == 24) / toneStack46);
	float toneStack61 = (4.7117500000000004e-07f * toneStack0);
	float toneStack62 = (0.00011998125000000002f * toneStack2);
	float toneStack63 = (5.718000000000001e-06f + (toneStack62 + (toneStack0 * (((1.1779375000000001e-05f * toneStack2) - 4.199450000000001e-06f) - toneStack61))));
	float toneStack64 = ((1.0281250000000001e-09f * toneStack2) - (4.1125e-11f * toneStack0));
	float toneStack65 = (7.343750000000001e-09f * toneStack2);
	float toneStack66 = (2.9375e-10f + (toneStack65 + (toneStack0 * (toneStack64 - 2.52625e-10f))));
	float toneStack67 = (tubefilter->fConst1 * toneStack66);
	float toneStack68 = (0.00047000000000000004f * toneStack0);
	float toneStack69 = (tubefilter->fConst1 * (0.015765f + (toneStack9 + toneStack68)));
	tubefilter->toneStack70 = ((toneStack69 + (tubefilter->fConst2 * (toneStack67 - toneStack63))) - 1.0f);
	float toneStack71 = (tubefilter->fConst3 * toneStack66);
	tubefilter->toneStack72 = ((tubefilter->fConst2 * (toneStack63 + toneStack71)) - (3.0f + toneStack69));
	tubefilter->toneStack73 = ((toneStack69 + (tubefilter->fConst2 * (toneStack63 - toneStack71))) - 3.0f);
	float toneStack74 = (-(1.0f + (toneStack69 + (tubefilter->fConst2 * (toneStack63 + toneStack67)))));
	tubefilter->toneStack75 = (1.0f / toneStack74);
	float toneStack76 = ((toneStack0 * (4.1125e-11f + toneStack64)) + (toneStack17 * ((2.9375e-10f - (2.9375e-10f * toneStack0)) + toneStack65)));
	float toneStack77 = (tubefilter->fConst3 * toneStack76);
	float toneStack78 = (9.187500000000001e-07f * toneStack17);
	float toneStack79 = (9.925e-08f + ((toneStack78 + (toneStack0 * (5.0055e-07f - toneStack61))) + (toneStack2 * (2.48125e-06f + (1.1779375000000001e-05f * toneStack0)))));
	float toneStack80 = (0.0010025f + (toneStack9 + (toneStack21 + toneStack68)));
	float toneStack81 = (tubefilter->fConst1 * toneStack80);
	tubefilter->toneStack82 = (toneStack81 + (tubefilter->fConst2 * (toneStack79 - toneStack77)));
	float toneStack83 = (tubefilter->fConst1 * toneStack76);
	tubefilter->toneStack84 = (toneStack81 + (tubefilter->fConst2 * (toneStack83 - toneStack79)));
	float toneStack85 = (tubefilter->fConst1 * (0 - toneStack80));
	tubefilter->toneStack86 = (toneStack85 + (tubefilter->fConst2 * (toneStack79 + toneStack77)));
	tubefilter->toneStack87 = (toneStack85 - (tubefilter->fConst2 * (toneStack79 + toneStack83)));
	tubefilter->toneStack88 = ((toneStack31 == 22) / toneStack74);
	float toneStack89 = (3.059375000000001e-07f * toneStack0);
	float toneStack90 = (1.5468750000000003e-06f + ((1.2718750000000003e-05f * toneStack2) + (toneStack0 * (((3.0593750000000007e-06f * toneStack2) - 8.696875000000003e-07f) - toneStack89))));
	float toneStack91 = ((2.646875e-10f * toneStack2) - (2.6468750000000002e-11f * toneStack0));
	float toneStack92 = (7.5625e-10f * toneStack2);
	float toneStack93 = (7.562500000000001e-11f + (toneStack92 + (toneStack0 * (toneStack91 - 4.915625000000001e-11f))));
	float toneStack94 = (tubefilter->fConst1 * toneStack93);
	float toneStack95 = (0.005562500000000001f * toneStack2);
	float toneStack96 = (tubefilter->fConst1 * (0.005018750000000001f + (toneStack8 + toneStack95)));
	tubefilter->toneStack97 = ((toneStack96 + (tubefilter->fConst2 * (toneStack94 - toneStack90))) - 1.0f);
	float toneStack98 = (tubefilter->fConst3 * toneStack93);
	tubefilter->toneStack99 = ((tubefilter->fConst2 * (toneStack90 + toneStack98)) - (3 + toneStack96));
	tubefilter->toneStack100 = ((toneStack96 + (tubefilter->fConst2 * (toneStack90 - toneStack98))) - 3.0f);
	float toneStack101 = (-(1 + (toneStack96 + (tubefilter->fConst2 * (toneStack90 + toneStack94)))));
	tubefilter->toneStack102 = (1.0f / toneStack101);
	float toneStack103 = ((toneStack0 * (2.6468750000000002e-11f + toneStack91)) + (toneStack17 * ((7.562500000000001e-11f - (7.562500000000001e-11f * toneStack0)) + toneStack92)));
	float toneStack104 = (tubefilter->fConst3 * toneStack103);
	float toneStack105 = (6.1875e-08f + (((2.75e-07f * toneStack17) + (toneStack0 * (3.403125000000001e-07f - toneStack89))) + (toneStack2 * (6.1875e-07f + (3.0593750000000007e-06f * toneStack0)))));
	float toneStack106 = (0.00055625f + (toneStack22 + toneStack95));
	float toneStack107 = (tubefilter->fConst1 * toneStack106);
	float toneStack108 = (toneStack107 + (tubefilter->fConst2 * (toneStack105 - toneStack104)));
	float toneStack109 = (tubefilter->fConst1 * toneStack103);
	tubefilter->toneStack110 = (toneStack107 + (tubefilter->fConst2 * (toneStack109 - toneStack105)));
	float toneStack111 = (tubefilter->fConst1 * -toneStack106);
	tubefilter->toneStack112 = (toneStack111 + (tubefilter->fConst2 * (toneStack105 + toneStack104)));
	tubefilter->toneStack113 = (toneStack111 - (tubefilter->fConst2 * (toneStack105 + toneStack109)));
	tubefilter->toneStack114 = ((toneStack31 == 21) / toneStack101);
	float toneStack115 = (2.2193400000000003e-07f * toneStack0);
	float toneStack116 = (2.7073879999999998e-06f + ((4.9553415999999996e-05f * toneStack2) + (toneStack0 * (((4.882548000000001e-06f * toneStack2) - 1.964318e-06f) - toneStack115))));
	float toneStack117 = ((3.4212992000000004e-10f * toneStack2) - (1.5551360000000004e-11f * toneStack0));
	float toneStack118 = (2.3521432000000003e-09f * toneStack2);
	float toneStack119 = (1.0691560000000001e-10f + (toneStack118 + (toneStack0 * (toneStack117 - 9.136424e-11f))));
	float toneStack120 = (tubefilter->fConst1 * toneStack119);
	float toneStack121 = (0.0103884f * toneStack2);
	float toneStack122 = (tubefilter->fConst1 * (0.009920600000000002f + (toneStack68 + toneStack121)));
	tubefilter->toneStack123 = ((toneStack122 + (tubefilter->fConst2 * (toneStack120 - toneStack116))) - 1.0f);
	float toneStack124 = (tubefilter->fConst3 * toneStack119);
	tubefilter->toneStack125 = ((tubefilter->fConst2 * (toneStack116 + toneStack124)) - (3 + toneStack122));
	tubefilter->toneStack126 = ((toneStack122 + (tubefilter->fConst2 * (toneStack116 - toneStack124))) - 3.0f);
	float toneStack127 = -(1.0f + (toneStack122 + (tubefilter->fConst2 * (toneStack116 + toneStack120))));
	tubefilter->toneStack128 = 1.0f / toneStack127;
	float toneStack129 = ((toneStack0 * (1.5551360000000004e-11f + toneStack117)) + (toneStack17 * ((1.0691560000000001e-10f - (1.0691560000000001e-10f * toneStack0)) + toneStack118)));
	float toneStack130 = (tubefilter->fConst3 * toneStack129);
	float toneStack131 = (4.3428e-08f + (((4.5496e-07f * toneStack17) + (toneStack0 * (2.4468200000000005e-07f - toneStack115))) + (toneStack2 * (9.55416e-07f + (4.882548000000001e-06f * toneStack0)))));
	float toneStack132 = (0.00047220000000000004f + (toneStack121 + (toneStack68 + (4.84e-05f * toneStack17))));
	float toneStack133 = (tubefilter->fConst1 * toneStack132);
	tubefilter->toneStack134 = (toneStack133 + (tubefilter->fConst2 * (toneStack131 - toneStack130)));
	float toneStack135 = (tubefilter->fConst1 * toneStack129);
	tubefilter->toneStack136 = (toneStack133 + (tubefilter->fConst2 * (toneStack135 - toneStack131)));
	float toneStack137 = (tubefilter->fConst1 * -toneStack132);
	tubefilter->toneStack138 = (toneStack137 + (tubefilter->fConst2 * (toneStack131 + toneStack130)));
	tubefilter->toneStack139 = (toneStack137 - (tubefilter->fConst2 * (toneStack131 + toneStack135)));
	tubefilter->toneStack140 = ((toneStack31 == 20) / toneStack127);
	float toneStack141 = (2.3926056000000006e-07f * toneStack0);
	float toneStack142 = (1.0875480000000001e-05f * toneStack2);
	float toneStack143 = (1.1144196800000003e-06f + ((3.659304000000001e-05f * toneStack2) + (toneStack0 * ((toneStack142 - 4.347578400000001e-07f) - toneStack141))));
	float toneStack144 = ((1.4413132800000006e-09f * toneStack2) - (3.1708892160000014e-11f * toneStack0));
	float toneStack145 = (3.403100800000001e-09f * toneStack2);
	float toneStack146 = (7.486821760000003e-11f + (toneStack145 + (toneStack0 * (toneStack144 - 4.315932544000001e-11f))));
	float toneStack147 = (tubefilter->fConst1 * toneStack146);
	float toneStack148 = (0.00048400000000000006f * toneStack0);
	float toneStack149 = (0.022470000000000004f * toneStack2);
	float toneStack150 = (toneStack149 + toneStack148);
	float toneStack151 = (tubefilter->fConst1 * (0.00358974f + toneStack150));
	tubefilter->toneStack152 = ((toneStack151 + (tubefilter->fConst2 * (toneStack147 - toneStack143))) - 1);
	float toneStack153 = (tubefilter->fConst3 * toneStack146);
	tubefilter->toneStack154 = ((tubefilter->fConst2 * (toneStack143 + toneStack153)) - (3 + toneStack151));
	tubefilter->toneStack155 = ((toneStack151 + (tubefilter->fConst2 * (toneStack143 - toneStack153))) - 3);
	float toneStack156 = (0 - (1 + (toneStack151 + (tubefilter->fConst2 * (toneStack143 + toneStack147)))));
	tubefilter->toneStack157 = (1.0f / toneStack156);
	float toneStack158 = ((toneStack0 * (3.1708892160000014e-11f + toneStack144)) + (toneStack17 * ((7.486821760000003e-11f - (7.486821760000003e-11f * toneStack0)) + toneStack145)));
	float toneStack159 = (tubefilter->fConst3 * toneStack158);
	float toneStack160 = (1.0875480000000001e-05f * toneStack0);
	float toneStack161 = (toneStack0 * (2.893061600000001e-07f - toneStack141));
	float toneStack162 = (8.098288000000002e-08f + (((3.0937280000000007e-07f * toneStack17) + toneStack161) + (toneStack2 * (3.6810400000000007e-06f + toneStack160))));
	float toneStack163 = (0.00049434f + (toneStack149 + (toneStack148 + (0.0001034f * toneStack17))));
	float toneStack164 = (tubefilter->fConst1 * toneStack163);
	tubefilter->toneStack165 = (toneStack164 + (tubefilter->fConst2 * (toneStack162 - toneStack159)));
	float toneStack166 = (tubefilter->fConst1 * toneStack158);
	tubefilter->toneStack167 = (toneStack164 + (tubefilter->fConst2 * (toneStack166 - toneStack162)));
	float toneStack168 = (tubefilter->fConst1 * -toneStack163);
	tubefilter->toneStack169 = (toneStack168 + (tubefilter->fConst2 * (toneStack162 + toneStack159)));
	tubefilter->toneStack170 = (toneStack168 - (tubefilter->fConst2 * (toneStack162 + toneStack166)));
	tubefilter->toneStack171 = ((toneStack31 == 19) / toneStack156);
	float toneStack172 = (7.790052600000002e-07f * toneStack0);
	float toneStack173 = (1.4106061200000003e-06f + ((3.7475640000000014e-05f * toneStack2) + (toneStack0 * (((2.3606220000000006e-05f * toneStack2) - 3.2220474e-07f) - toneStack172))));
	float toneStack174 = ((1.5406083e-09f * toneStack2) - (5.08400739e-11f * toneStack0));
	float toneStack175 = (1.9775250000000004e-09f * toneStack2);
	float toneStack176 = (6.5258325e-11f + (toneStack175 + (toneStack0 * (toneStack174 - 1.4418251099999996e-11f))));
	float toneStack177 = (tubefilter->fConst1 * toneStack176);
	float toneStack178 = (0.001551f * toneStack0);
	float toneStack179 = (0.015220000000000001f * toneStack2);
	float toneStack180 = (tubefilter->fConst1 * (0.0037192600000000003f + (toneStack179 + toneStack178)));
	tubefilter->toneStack181 = ((toneStack180 + (tubefilter->fConst2 * (toneStack177 - toneStack173))) - 1);
	float toneStack182 = (tubefilter->fConst3 * toneStack176);
	tubefilter->toneStack183 = ((tubefilter->fConst2 * (toneStack173 + toneStack182)) - (3 + toneStack180));
	tubefilter->toneStack184 = ((toneStack180 + (tubefilter->fConst2 * (toneStack173 - toneStack182))) - 3);
	float toneStack185 = (0 - (1 + (toneStack180 + (tubefilter->fConst2 * (toneStack173 + toneStack177)))));
	tubefilter->toneStack186 = (1.0f / toneStack185);
	float toneStack187 = ((toneStack0 * (5.08400739e-11f + toneStack174)) + (toneStack17 * ((6.5258325e-11f - (6.5258325e-11f * toneStack0)) + toneStack175)));
	float toneStack188 = (tubefilter->fConst3 * toneStack187);
	float toneStack189 = (5.018112e-08f + (((1.7391e-07f * toneStack17) + (toneStack0 * (8.643102600000002e-07f - toneStack172)))
		+ (toneStack2 * (1.5206400000000001e-06f + (2.3606220000000006e-05f * toneStack0)))));
	float toneStack190 = (0.0005022600000000001f + (toneStack179 + (toneStack178 + (5.4999999999999995e-05f * toneStack17))));
	float toneStack191 = (tubefilter->fConst1 * toneStack190);
	tubefilter->toneStack192 = (toneStack191 + (tubefilter->fConst2 * (toneStack189 - toneStack188)));
	float toneStack193 = (tubefilter->fConst1 * toneStack187);
	tubefilter->toneStack194 = (toneStack191 + (tubefilter->fConst2 * (toneStack193 - toneStack189)));
	float toneStack195 = (tubefilter->fConst1 * -toneStack190);
	tubefilter->toneStack196 = (toneStack195 + (tubefilter->fConst2 * (toneStack189 + toneStack188)));
	tubefilter->toneStack197 = (toneStack195 - (tubefilter->fConst2 * (toneStack189 + toneStack193)));
	tubefilter->toneStack198 = ((toneStack31 == 18) / toneStack185);
	float toneStack199 = (4.7047000000000006e-07f * toneStack0);
	float toneStack200 = (5.107200000000001e-06f + ((0.00011849250000000002f * toneStack2) + (toneStack0 * (((1.1761750000000001e-05f * toneStack2) - 4.217780000000001e-06f) - toneStack199))));
	float toneStack201 = ((4.1125e-10f * toneStack2) - (1.645e-11f * toneStack0));
	float toneStack202 = (2.9375000000000002e-09f * toneStack2);
	float toneStack203 = (1.175e-10f + (toneStack202 + (toneStack0 * (toneStack201 - 1.0105e-10f))));
	float toneStack204 = (tubefilter->fConst1 * toneStack203);
	float toneStack205 = (0.025025000000000002f * toneStack2);
	float toneStack206 = (tubefilter->fConst1 * (0.015726f + (toneStack68 + toneStack205)));
	tubefilter->toneStack207 = ((toneStack206 + (tubefilter->fConst2 * (toneStack204 - toneStack200))) - 1);
	float toneStack208 = (tubefilter->fConst3 * toneStack203);
	tubefilter->toneStack209 = ((tubefilter->fConst2 * (toneStack200 + toneStack208)) - (3 + toneStack206));
	tubefilter->toneStack210 = ((toneStack206 + (tubefilter->fConst2 * (toneStack200 - toneStack208))) - 3);
	float toneStack211 = (0 - (1 + (toneStack206 + (tubefilter->fConst2 * (toneStack200 + toneStack204)))));
	tubefilter->toneStack212 = (1.0f / toneStack211);
	float toneStack213 = ((toneStack0 * (1.645e-11f + toneStack201)) + (toneStack17 * ((1.175e-10f - (1.175e-10f * toneStack0)) + toneStack202)));
	float toneStack214 = (tubefilter->fConst3 * toneStack213);
	float toneStack215 = (3.9700000000000005e-08f + (((3.675000000000001e-07f * toneStack17) + (toneStack0 * (4.8222e-07f - toneStack199)))
		+ (toneStack2 * (9.925e-07f + (1.1761750000000001e-05f * toneStack0)))));
	float toneStack216 = (0.001001f + (toneStack205 + (toneStack51 + toneStack68)));
	float toneStack217 = (tubefilter->fConst1 * toneStack216);
	tubefilter->toneStack218 = (toneStack217 + (tubefilter->fConst2 * (toneStack215 - toneStack214)));
	float toneStack219 = (tubefilter->fConst1 * toneStack213);
	tubefilter->toneStack220 = (toneStack217 + (tubefilter->fConst2 * (toneStack219 - toneStack215)));
	float toneStack221 = (tubefilter->fConst1 * (0 - toneStack216));
	tubefilter->toneStack222 = (toneStack221 + (tubefilter->fConst2 * (toneStack215 + toneStack214)));
	tubefilter->toneStack223 = (toneStack221 - (tubefilter->fConst2 * (toneStack215 + toneStack219)));
	tubefilter->toneStack224 = ((toneStack31 == 17) / toneStack211);
	float toneStack225 = (3.0896250000000005e-07f * toneStack0);
	float toneStack226 = (6.338090000000001e-07f + ((1.8734760000000003e-05f * toneStack2) + (toneStack0 * (((1.2358500000000002e-05f * toneStack2) - 1.361249999999999e-08f) - toneStack225))));
	float toneStack227 = ((1.6037340000000005e-09f * toneStack2) - (4.0093350000000015e-11f * toneStack0));
	float toneStack228 = (1.8198400000000004e-09f * toneStack2);
	float toneStack229 = (4.5496000000000015e-11f + (toneStack228 + (toneStack0 * (toneStack227 - 5.40265e-12f))));
	float toneStack230 = (tubefilter->fConst1 * toneStack229);
	float toneStack231 = (tubefilter->fConst1 * (0.00208725f + (toneStack8 + toneStack149)));
	tubefilter->toneStack232 = ((toneStack231 + (tubefilter->fConst2 * (toneStack230 - toneStack226))) - 1);
	float toneStack233 = (tubefilter->fConst3 * toneStack229);
	tubefilter->toneStack234 = ((tubefilter->fConst2 * (toneStack226 + toneStack233)) - (3 + toneStack231));
	tubefilter->toneStack235 = ((toneStack231 + (tubefilter->fConst2 * (toneStack226 - toneStack233))) - 3);
	float toneStack236 = (0 - (1 + (toneStack231 + (tubefilter->fConst2 * (toneStack226 + toneStack230)))));
	tubefilter->toneStack237 = (1.0f / toneStack236);
	float toneStack238 = ((toneStack0 * (4.0093350000000015e-11f + toneStack227)) + (toneStack17 * ((4.5496000000000015e-11f - (4.5496000000000015e-11f * toneStack0)) + toneStack228)));
	float toneStack239 = (tubefilter->fConst3 * toneStack238);
	float toneStack240 = (8.1169e-08f + (((1.6544000000000003e-07f * toneStack17) + (toneStack0 * (3.735875000000001e-07f - toneStack225))) + (toneStack2 * (3.24676e-06f + (1.2358500000000002e-05f * toneStack0)))));
	float toneStack241 = (0.00011750000000000001f * toneStack17);
	float toneStack242 = (0.0005617500000000001f + (toneStack149 + (toneStack8 + toneStack241)));
	float toneStack243 = (tubefilter->fConst1 * toneStack242);
	float toneStack244 = (toneStack243 + (tubefilter->fConst2 * (toneStack240 - toneStack239)));
	float toneStack245 = (tubefilter->fConst1 * toneStack238);
	tubefilter->toneStack246 = (toneStack243 + (tubefilter->fConst2 * (toneStack245 - toneStack240)));
	float toneStack247 = (tubefilter->fConst1 * -toneStack242);
	tubefilter->toneStack248 = (toneStack247 + (tubefilter->fConst2 * (toneStack240 + toneStack239)));
	tubefilter->toneStack249 = (toneStack247 - (tubefilter->fConst2 * (toneStack240 + toneStack245)));
	tubefilter->toneStack250 = ((toneStack31 == 16) / toneStack236);
	float toneStack251 = (2.7256800000000006e-07f * toneStack0);
	float toneStack252 = (1.4234760000000002e-06f + ((2.851440000000001e-05f * toneStack2) + (toneStack0 * (((6.8142000000000025e-06f * toneStack2) - 7.876920000000001e-07f) - toneStack251))));
	float toneStack253 = ((4.724676000000001e-10f * toneStack2) - (1.8898704000000002e-11f * toneStack0));
	float toneStack254 = (1.6641900000000002e-09f * toneStack2);
	float toneStack255 = (6.656760000000001e-11f + (toneStack254 + (toneStack0 * (toneStack253 - 4.7668896000000004e-11f))));
	float toneStack256 = (tubefilter->fConst1 * toneStack255);
	float toneStack257 = (0.0008200000000000001f * toneStack0);
	float toneStack258 = (0.00831f * toneStack2);
	float toneStack259 = (tubefilter->fConst1 * (0.005107400000000001f + (toneStack258 + toneStack257)));
	tubefilter->toneStack260 = ((toneStack259 + (tubefilter->fConst2 * (toneStack256 - toneStack252))) - 1);
	float toneStack261 = (tubefilter->fConst3 * toneStack255);
	tubefilter->toneStack262 = ((tubefilter->fConst2 * (toneStack252 + toneStack261)) - (3 + toneStack259));
	tubefilter->toneStack263 = ((toneStack259 + (tubefilter->fConst2 * (toneStack252 - toneStack261))) - 3);
	float toneStack264 = (0 - (1 + (toneStack259 + (tubefilter->fConst2 * (toneStack252 + toneStack256)))));
	tubefilter->toneStack265 = (1.0f / toneStack264);
	float toneStack266 = ((toneStack0 * (1.8898704000000002e-11f + toneStack253)) + (toneStack17 * ((6.656760000000001e-11f - (6.656760000000001e-11f * toneStack0)) + toneStack254)));
	float toneStack267 = (tubefilter->fConst3 * toneStack266);
	float toneStack268 = (3.1116000000000005e-08f + (((2.829e-07f * toneStack17) + (toneStack0 * (3.2176800000000005e-07f - toneStack251))) + (toneStack2 * (7.779000000000002e-07f + (6.8142000000000025e-06f * toneStack0)))));
	float toneStack269 = (0.00033240000000000006f + (toneStack258 + (toneStack257 + (6e-05f * toneStack17))));
	float toneStack270 = (tubefilter->fConst1 * toneStack269);
	tubefilter->toneStack271 = (toneStack270 + (tubefilter->fConst2 * (toneStack268 - toneStack267)));
	float toneStack272 = (tubefilter->fConst1 * toneStack266);
	tubefilter->toneStack273 = (toneStack270 + (tubefilter->fConst2 * (toneStack272 - toneStack268)));
	float toneStack274 = (tubefilter->fConst1 * -toneStack269);
	tubefilter->toneStack275 = (toneStack274 + (tubefilter->fConst2 * (toneStack268 + toneStack267)));
	tubefilter->toneStack276 = (toneStack274 - (tubefilter->fConst2 * (toneStack268 + toneStack272)));
	tubefilter->toneStack277 = ((toneStack31 == 15) / toneStack264);
	float toneStack278 = (4.0108000000000004e-07f * toneStack0);
	float toneStack279 = (5.050300000000001e-06f + ((0.00010263250000000001f * toneStack2) + (toneStack0 * (((1.0027e-05f * toneStack2) - 3.5719200000000006e-06f) - toneStack278))));
	float toneStack280 = ((9.45e-10f * toneStack2) - (3.78e-11f * toneStack0));
	float toneStack281 = (6.75e-09f * toneStack2);
	float toneStack282 = (2.7e-10f + (toneStack281 + (toneStack0 * (toneStack280 - 2.3219999999999998e-10f))));
	float toneStack283 = (tubefilter->fConst1 * toneStack282);
	float toneStack284 = (0.0004f * toneStack0);
	float toneStack285 = (0.025067500000000003f * toneStack2);
	float toneStack286 = (tubefilter->fConst1 * (0.0150702f + (toneStack285 + toneStack284)));
	tubefilter->toneStack287 = ((toneStack286 + (tubefilter->fConst2 * (toneStack283 - toneStack279))) - 1);
	float toneStack288 = (tubefilter->fConst3 * toneStack282);
	tubefilter->toneStack289 = ((tubefilter->fConst2 * (toneStack279 + toneStack288)) - (3 + toneStack286));
	tubefilter->toneStack290 = ((toneStack286 + (tubefilter->fConst2 * (toneStack279 - toneStack288))) - 3);
	float toneStack291 = (0 - (1 + (toneStack286 + (tubefilter->fConst2 * (toneStack279 + toneStack283)))));
	tubefilter->toneStack292 = (1.0f / toneStack291);
	float toneStack293 = ((toneStack0 * (3.78e-11f + toneStack280)) + (toneStack17 * ((2.7e-10f - (2.7e-10f * toneStack0)) + toneStack281)));
	float toneStack294 = (tubefilter->fConst3 * toneStack293);
	float toneStack295 = (1.0530000000000001e-07f + (((9.45e-07f * toneStack17) + (toneStack0 * (4.2808000000000006e-07f - toneStack278))) + (toneStack2 * (2.6324999999999998e-06f + (1.0027e-05f * toneStack0)))));
	float toneStack296 = (6.75e-05f * toneStack17);
	float toneStack297 = (0.0010027f + (toneStack285 + (toneStack284 + toneStack296)));
	float toneStack298 = (tubefilter->fConst1 * toneStack297);
	tubefilter->toneStack299 = (toneStack298 + (tubefilter->fConst2 * (toneStack295 - toneStack294)));
	float toneStack300 = (tubefilter->fConst1 * toneStack293);
	tubefilter->toneStack301 = (toneStack298 + (tubefilter->fConst2 * (toneStack300 - toneStack295)));
	float toneStack302 = (tubefilter->fConst1 * -toneStack297);
	tubefilter->toneStack303 = (toneStack302 + (tubefilter->fConst2 * (toneStack295 + toneStack294)));
	tubefilter->toneStack304 = (toneStack302 - (tubefilter->fConst2 * (toneStack295 + toneStack300)));
	tubefilter->toneStack305 = ((toneStack31 == 14) / toneStack291);
	float toneStack306 = (1.95976e-07f * toneStack0);
	float toneStack307 = (9.060568000000001e-07f + ((8.801210000000002e-06f * toneStack2) + (toneStack0 * (((2.4497000000000004e-06f * toneStack2) - 4.3256399999999996e-07f) - toneStack306))));
	float toneStack308 = ((2.0778120000000008e-10f * toneStack2) - (1.6622496000000003e-11f * toneStack0));
	float toneStack309 = (5.553900000000002e-10f * toneStack2);
	float toneStack310 = (4.4431200000000016e-11f + (toneStack309 + (toneStack0 * (toneStack308 - 2.7808704000000013e-11f))));
	float toneStack311 = (tubefilter->fConst1 * toneStack310);
	float toneStack312 = (0.00044f * toneStack0);
	float toneStack313 = (0.0055675f * toneStack2);
	float toneStack314 = (tubefilter->fConst1 * (0.0035049f + (toneStack313 + toneStack312)));
	tubefilter->toneStack315 = ((toneStack314 + (tubefilter->fConst2 * (toneStack311 - toneStack307))) - 1);
	float toneStack316 = (tubefilter->fConst3 * toneStack310);
	tubefilter->toneStack317 = ((tubefilter->fConst2 * (toneStack307 + toneStack316)) - (3 + toneStack314));
	tubefilter->toneStack318 = ((toneStack314 + (tubefilter->fConst2 * (toneStack307 - toneStack316))) - 3);
	float toneStack319 = (0 - (1 + (toneStack314 + (tubefilter->fConst2 * (toneStack307 + toneStack311)))));
	tubefilter->toneStack320 = (1.0f / toneStack319);
	float toneStack321 = ((toneStack0 * (1.6622496000000003e-11f + toneStack308)) + (toneStack17 * ((4.4431200000000016e-11f - (4.4431200000000016e-11f * toneStack0)) + toneStack309)));
	float toneStack322 = (tubefilter->fConst3 * toneStack321);
	float toneStack323 = (4.585680000000001e-08f + (((2.0196000000000004e-07f * toneStack17) + (toneStack0 * (2.2567600000000002e-07f - toneStack306))) + (toneStack2 * (5.732100000000001e-07f + (2.4497000000000004e-06f * toneStack0)))));
	float toneStack324 = (0.00044540000000000004f + (toneStack313 + (toneStack296 + toneStack312)));
	float toneStack325 = (tubefilter->fConst1 * toneStack324);
	tubefilter->toneStack326 = (toneStack325 + (tubefilter->fConst2 * (toneStack323 - toneStack322)));
	float toneStack327 = (tubefilter->fConst1 * toneStack321);
	tubefilter->toneStack328 = (toneStack325 + (tubefilter->fConst2 * (toneStack327 - toneStack323)));
	float toneStack329 = (tubefilter->fConst1 * (0 - toneStack324));
	tubefilter->toneStack330 = (toneStack329 + (tubefilter->fConst2 * (toneStack323 + toneStack322)));
	tubefilter->toneStack331 = (toneStack329 - (tubefilter->fConst2 * (toneStack323 + toneStack327)));
	tubefilter->toneStack332 = ((toneStack31 == 13) / toneStack319);
	float toneStack333 = (4.9434000000000004e-08f * toneStack0);
	float toneStack334 = (7.748796000000001e-07f + ((2.8889960000000004e-05f * toneStack2) + (toneStack0 * (((4.943400000000001e-06f * toneStack2) - 1.2634599999999999e-07f) - toneStack333))));
	float toneStack335 = ((1.2443156000000004e-09f * toneStack2) - (1.2443156000000002e-11f * toneStack0));
	float toneStack336 = (5.345780000000001e-09f * toneStack2);
	float toneStack337 = (5.345780000000001e-11f + (toneStack336 + (toneStack0 * (toneStack335 - 4.101464400000001e-11f))));
	float toneStack338 = (tubefilter->fConst1 * toneStack337);
	float toneStack339 = (0.00022f * toneStack0);
	float toneStack340 = (tubefilter->fConst1 * (0.0025277f + (toneStack149 + toneStack339)));
	tubefilter->toneStack341 = ((toneStack340 + (tubefilter->fConst2 * (toneStack338 - toneStack334))) - 1.0f);
	float toneStack342 = (tubefilter->fConst3 * toneStack337);
	tubefilter->toneStack343 = ((tubefilter->fConst2 * (toneStack334 + toneStack342)) - (3 + toneStack340));
	tubefilter->toneStack344 = ((toneStack340 + (tubefilter->fConst2 * (toneStack334 - toneStack342))) - 3.0f);
	float toneStack345 = (0 - (1 + (toneStack340 + (tubefilter->fConst2 * (toneStack334 + toneStack338)))));
	tubefilter->toneStack346 = (1.0f / toneStack345);
	float toneStack347 = ((toneStack0 * (1.2443156000000002e-11f + toneStack335)) + (toneStack17 * ((5.345780000000001e-11f - (5.345780000000001e-11f * toneStack0)) + toneStack336)));
	float toneStack348 = (tubefilter->fConst3 * toneStack347);
	float toneStack349 = (6.141960000000001e-08f + (((4.859800000000001e-07f * toneStack17) + (toneStack0 * (1.0113400000000001e-07f - toneStack333))) + (toneStack2 * (6.141960000000001e-06f
		+ (4.943400000000001e-06f * toneStack0)))));
	float toneStack350 = (0.00022470000000000001f + (toneStack149 + (toneStack339 + (0.00023500000000000002f * toneStack17))));
	float toneStack351 = (tubefilter->fConst1 * toneStack350);
	tubefilter->toneStack352 = (toneStack351 + (tubefilter->fConst2 * (toneStack349 - toneStack348)));
	float toneStack353 = (tubefilter->fConst1 * toneStack347);
	tubefilter->toneStack354 = (toneStack351 + (tubefilter->fConst2 * (toneStack353 - toneStack349)));
	float toneStack355 = (tubefilter->fConst1 * (0 - toneStack350));
	tubefilter->toneStack356 = (toneStack355 + (tubefilter->fConst2 * (toneStack349 + toneStack348)));
	tubefilter->toneStack357 = (toneStack355 - (tubefilter->fConst2 * (toneStack349 + toneStack353)));
	tubefilter->toneStack358 = ((toneStack31 == 12) / toneStack345);
	float toneStack359 = (2.5587500000000006e-07f * toneStack0);
	float toneStack360 = (7.717400000000001e-07f + ((2.2033600000000005e-05f * toneStack2) + (toneStack0 * (((1.0235000000000001e-05f * toneStack2) - 1.5537499999999997e-07f) - toneStack359))));
	float toneStack361 = ((1.3959000000000001e-09f * toneStack2) - (3.48975e-11f * toneStack0));
	float toneStack362 = (2.2090000000000005e-09f * toneStack2);
	float toneStack363 = (5.522500000000001e-11f + (toneStack362 + (toneStack0 * (toneStack361 - 2.0327500000000007e-11f))));
	float toneStack364 = (tubefilter->fConst1 * toneStack363);
	float toneStack365 = (0.0005f * toneStack0);
	float toneStack366 = (0.020470000000000002f * toneStack2);
	float toneStack367 = (tubefilter->fConst1 * (0.0025092499999999998f + (toneStack366 + toneStack365)));
	tubefilter->toneStack368 = ((toneStack367 + (tubefilter->fConst2 * (toneStack364 - toneStack360))) - 1);
	float toneStack369 = (tubefilter->fConst3 * toneStack363);
	tubefilter->toneStack370 = ((tubefilter->fConst2 * (toneStack360 + toneStack369)) - (3 + toneStack367));
	tubefilter->toneStack371 = ((toneStack367 + (tubefilter->fConst2 * (toneStack360 - toneStack369))) - 3);
	float toneStack372 = (-(1.0f + (toneStack367 + (tubefilter->fConst2 * (toneStack360 + toneStack364)))));
	tubefilter->toneStack373 = (1.0f / toneStack372);
	float toneStack374 = ((toneStack0 * (3.48975e-11f + toneStack361)) + (toneStack17 * ((5.522500000000001e-11f - (5.522500000000001e-11f * toneStack0)) + toneStack362)));
	float toneStack375 = (tubefilter->fConst3 * toneStack374);
	float toneStack376 = (8.084000000000001e-08f + (((2.2090000000000003e-07f * toneStack17) + (toneStack0 * (3.146250000000001e-07f - toneStack359)))
		+ (toneStack2 * (3.2336000000000007e-06f + (1.0235000000000001e-05f * toneStack0)))));
	float toneStack377 = (0.00051175f + (toneStack366 + (toneStack241 + toneStack365)));
	float toneStack378 = (tubefilter->fConst1 * toneStack377);
	tubefilter->toneStack379 = (toneStack378 + (tubefilter->fConst2 * (toneStack376 - toneStack375)));
	float toneStack380 = (tubefilter->fConst1 * toneStack374);
	tubefilter->toneStack381 = (toneStack378 + (tubefilter->fConst2 * (toneStack380 - toneStack376)));
	float toneStack382 = (tubefilter->fConst1 * -toneStack377);
	tubefilter->toneStack383 = (toneStack382 + (tubefilter->fConst2 * (toneStack376 + toneStack375)));
	tubefilter->toneStack384 = (toneStack382 - (tubefilter->fConst2 * (toneStack376 + toneStack380)));
	tubefilter->toneStack385 = ((toneStack31 == 11) / toneStack372);
	float toneStack386 = (0.00022854915600000004f * toneStack0);
	float toneStack387 = (0.00010871476000000002f + ((0.00010719478000000002f * toneStack2) + (toneStack0 * ((0.00012621831200000002f + (0.00022854915600000004f * toneStack2)) - toneStack386))));
	float toneStack388 = ((3.421299200000001e-08f * toneStack2) - (3.421299200000001e-08f * toneStack0));
	float toneStack389 = (1.0f + (toneStack2 + (93531720.34763868f * (toneStack0 * (2.3521432000000005e-08f + toneStack388)))));
	float toneStack390 = (tubefilter->fConst4 * toneStack389);
	float toneStack391 = (tubefilter->fConst1 * (0.036906800000000003f + ((0.022103400000000002f * toneStack2) + (0.01034f * toneStack0))));
	tubefilter->toneStack392 = ((toneStack391 + (tubefilter->fConst2 * (toneStack390 - toneStack387))) - 1.0f);
	float toneStack393 = (tubefilter->fConst5 * toneStack389);
	tubefilter->toneStack394 = ((tubefilter->fConst2 * (toneStack387 + toneStack393)) - (3.0f + toneStack391));
	tubefilter->toneStack395 = ((toneStack391 + (tubefilter->fConst2 * (toneStack387 - toneStack393))) - 3.0f);
	float toneStack396 = ((tubefilter->fConst2 * (-(toneStack387 + toneStack390))) - (1.0f + toneStack391));
	tubefilter->toneStack397 = (1.0f / toneStack396);
	float toneStack398 = ((toneStack0 * (3.421299200000001e-08f + toneStack388)) + (toneStack17 * ((1.0691560000000003e-08f - (1.0691560000000003e-08f * toneStack0)) + (1.0691560000000003e-08f * toneStack2))));
	float toneStack399 = (tubefilter->fConst3 * toneStack398);
	float toneStack400 = (3.7947800000000004e-06f + (((1.5199800000000001e-06f * toneStack17) + (toneStack0 * (0.00022961831200000004f - toneStack386))) + (toneStack2 * (3.7947800000000004e-06f + toneStack386))));
	float toneStack401 = (1.0f + (toneStack2 + ((0.0046780133373146215f * toneStack17) + (0.4678013337314621f * toneStack0))));
	float toneStack402 = (tubefilter->fConst6 * toneStack401);
	tubefilter->toneStack403 = (toneStack402 + (tubefilter->fConst2 * (toneStack400 - toneStack399)));
	float toneStack404 = (tubefilter->fConst1 * toneStack398);
	tubefilter->toneStack405 = (toneStack402 + (tubefilter->fConst2 * (toneStack404 - toneStack400)));
	float toneStack406 = (tubefilter->fConst1 * (0 - (0.022103400000000002f * toneStack401)));
	tubefilter->toneStack407 = (toneStack406 + (tubefilter->fConst2 * (toneStack400 + toneStack399)));
	tubefilter->toneStack408 = (toneStack406 - (tubefilter->fConst2 * (toneStack400 + toneStack404)));
	tubefilter->toneStack409 = ((toneStack31 == 10) / toneStack396);
	float toneStack410 = (4.851e-08f * toneStack0);
	float toneStack411 = (7.172000000000001e-07f + ((4.972000000000001e-05f * toneStack2) + (toneStack0 * (((4.8510000000000015e-06f * toneStack2) - 4.2449000000000006e-07f) - toneStack410))));
	float toneStack412 = ((2.6620000000000007e-10f * toneStack2) - (2.662e-12f * toneStack0));
	float toneStack413 = (2.4200000000000003e-09f * toneStack2);
	float toneStack414 = (2.4200000000000004e-11f + (toneStack413 + (toneStack0 * (toneStack412 - 2.1538000000000003e-11f))));
	float toneStack415 = (tubefilter->fConst1 * toneStack414);
	float toneStack416 = (0.022050000000000004f * toneStack2);
	float toneStack417 = (tubefilter->fConst1 * (0.0046705f + (toneStack339 + toneStack416)));
	tubefilter->toneStack418 = ((toneStack417 + (tubefilter->fConst2 * (toneStack415 - toneStack411))) - 1.0f);
	float toneStack419 = (tubefilter->fConst3 * toneStack414);
	tubefilter->toneStack420 = ((tubefilter->fConst2 * (toneStack411 + toneStack419)) - (3.0f + toneStack417));
	tubefilter->toneStack421 = ((toneStack417 + (tubefilter->fConst2 * (toneStack411 - toneStack419))) - 3.0f);
	float toneStack422 = (-(1.0f + (toneStack417 + (tubefilter->fConst2 * (toneStack411 + toneStack415)))));
	tubefilter->toneStack423 = (1.0f / toneStack422);
	float toneStack424 = ((toneStack0 * (2.662e-12f + toneStack412)) + (toneStack17 * ((2.4200000000000004e-11f - (2.4200000000000004e-11f * toneStack0)) + toneStack413)));
	float toneStack425 = (tubefilter->fConst3 * toneStack424);
	float toneStack426 = (1.32e-08f + (((2.2000000000000004e-07f * toneStack17) + (toneStack0 * (5.951000000000001e-08f - toneStack410))) + (toneStack2 * (1.32e-06f + (4.8510000000000015e-06f * toneStack0)))));
	float toneStack427 = (0.00022050000000000002f + (toneStack416 + (toneStack339 + (5e-05f * toneStack17))));
	float toneStack428 = (tubefilter->fConst1 * toneStack427);
	tubefilter->toneStack429 = (toneStack428 + (tubefilter->fConst2 * (toneStack426 - toneStack425)));
	float toneStack430 = (tubefilter->fConst1 * toneStack424);
	tubefilter->toneStack431 = (toneStack428 + (tubefilter->fConst2 * (toneStack430 - toneStack426)));
	float toneStack432 = (tubefilter->fConst1 * (0 - toneStack427));
	tubefilter->toneStack433 = (toneStack432 + (tubefilter->fConst2 * (toneStack426 + toneStack425)));
	tubefilter->toneStack434 = (toneStack432 - (tubefilter->fConst2 * (toneStack426 + toneStack430)));
	tubefilter->toneStack435 = ((toneStack31 == 9) / toneStack422);
	float toneStack436 = (1.38796875e-06f * toneStack0);
	float toneStack437 = (3.5279375000000002e-06f + ((3.1989375e-05f * toneStack2) + (toneStack0 * (((1.38796875e-05f * toneStack2) - 1.6311937500000001e-06f) - toneStack436))));
	float toneStack438 = ((1.0561781250000004e-09f * toneStack2) - (1.0561781250000003e-10f * toneStack0));
	float toneStack439 = (1.9328750000000005e-09f * toneStack2);
	float toneStack440 = (1.9328750000000007e-10f + (toneStack439 + (toneStack0 * (toneStack438 - 8.766968750000004e-11f))));
	float toneStack441 = (tubefilter->fConst1 * toneStack440);
	float toneStack442 = (0.001175f * toneStack0);
	float toneStack443 = (0.011812500000000002f * toneStack2);
	float toneStack444 = (tubefilter->fConst1 * (0.0065077500000000005f + (toneStack443 + toneStack442)));
	tubefilter->toneStack445 = ((toneStack444 + (tubefilter->fConst2 * (toneStack441 - toneStack437))) - 1);
	float toneStack446 = (tubefilter->fConst3 * toneStack440);
	tubefilter->toneStack447 = ((tubefilter->fConst2 * (toneStack437 + toneStack446)) - (3 + toneStack444));
	tubefilter->toneStack448 = ((toneStack444 + (tubefilter->fConst2 * (toneStack437 - toneStack446))) - 3);
	float toneStack449 = (0 - (1 + (toneStack444 + (tubefilter->fConst2 * (toneStack437 + toneStack441)))));
	tubefilter->toneStack450 = (1.0f / toneStack449);
	float toneStack451 = ((toneStack0 * (1.0561781250000003e-10f + toneStack438)) + (toneStack17 * ((1.9328750000000007e-10f - (1.9328750000000007e-10f * toneStack0)) + toneStack439)));
	float toneStack452 = (tubefilter->fConst3 * toneStack451);
	float toneStack453 = (1.0633750000000002e-07f + (((3.2900000000000005e-07f * toneStack17) + (toneStack0 * (1.4614062500000001e-06f - toneStack436)))
		+ (toneStack2 * (1.0633750000000002e-06f + (1.38796875e-05f * toneStack0)))));
	float toneStack454 = (toneStack21 + toneStack442);
	float toneStack455 = (0.00118125f + (toneStack443 + toneStack454));
	float toneStack456 = (tubefilter->fConst1 * toneStack455);
	tubefilter->toneStack457 = (toneStack456 + (tubefilter->fConst2 * (toneStack453 - toneStack452)));
	float toneStack458 = (tubefilter->fConst1 * toneStack451);
	tubefilter->toneStack459 = (toneStack456 + (tubefilter->fConst2 * (toneStack458 - toneStack453)));
	float toneStack460 = (tubefilter->fConst1 * -toneStack455);
	tubefilter->toneStack461 = (toneStack460 + (tubefilter->fConst2 * (toneStack453 + toneStack452)));
	tubefilter->toneStack462 = (toneStack460 - (tubefilter->fConst2 * (toneStack453 + toneStack458)));
	tubefilter->toneStack463 = ((toneStack31 == 8) / toneStack449);
	float toneStack464 = (3.0937500000000006e-07f * toneStack0);
	float toneStack465 = (1.2375000000000003e-05f * toneStack2);
	float toneStack466 = (6.677000000000001e-07f + ((1.9448000000000004e-05f * toneStack2) + (toneStack0 * ((toneStack465 - 2.1175000000000003e-08f) - toneStack464))));
	float toneStack467 = ((1.7121500000000001e-09f * toneStack2) - (4.2803750000000003e-11f * toneStack0));
	float toneStack468 = (1.9965000000000003e-09f * toneStack2);
	float toneStack469 = (4.991250000000001e-11f + (toneStack468 + (toneStack0 * (toneStack467 - 7.108750000000004e-12f))));
	float toneStack470 = (tubefilter->fConst1 * toneStack469);
	float toneStack471 = (0.022500000000000003f * toneStack2);
	float toneStack472 = (toneStack8 + toneStack471);
	float toneStack473 = (tubefilter->fConst1 * (0.0021395000000000003f + toneStack472));
	tubefilter->toneStack474 = ((toneStack473 + (tubefilter->fConst2 * (toneStack470 - toneStack466))) - 1.0f);
	float toneStack475 = (tubefilter->fConst3 * toneStack469);
	tubefilter->toneStack476 = ((tubefilter->fConst2 * (toneStack466 + toneStack475)) - (3.0f + toneStack473));
	tubefilter->toneStack477 = ((toneStack473 + (tubefilter->fConst2 * (toneStack466 - toneStack475))) - 3.0f);
	float toneStack478 = (-(1.0f + (toneStack473 + (tubefilter->fConst2 * (toneStack466 + toneStack470)))));
	tubefilter->toneStack479 = (1.0f / toneStack478);
	float toneStack480 = ((toneStack0 * (4.2803750000000003e-11f + toneStack467)) + (toneStack17 * ((4.991250000000001e-11f - (4.991250000000001e-11f * toneStack0)) + toneStack468)));
	float toneStack481 = (tubefilter->fConst3 * toneStack480);
	float toneStack482 = (1.2375000000000003e-05f * toneStack0);
	float toneStack483 = (toneStack0 * (3.781250000000001e-07f - toneStack464));
	float toneStack484 = (8.690000000000002e-08f + (((1.815e-07f * toneStack17) + toneStack483) + (toneStack2 * (3.4760000000000007e-06f + toneStack482))));
	float toneStack485 = (0.0005625000000000001f + (toneStack471 + (toneStack8 + (0.000125f * toneStack17))));
	float toneStack486 = (tubefilter->fConst1 * toneStack485);
	tubefilter->toneStack487 = (toneStack486 + (tubefilter->fConst2 * (toneStack484 - toneStack481)));
	float toneStack488 = (tubefilter->fConst1 * toneStack480);
	tubefilter->toneStack489 = (toneStack486 + (tubefilter->fConst2 * (toneStack488 - toneStack484)));
	float toneStack490 = (tubefilter->fConst1 * -toneStack485);
	tubefilter->toneStack491 = (toneStack490 + (tubefilter->fConst2 * (toneStack484 + toneStack481)));
	tubefilter->toneStack492 = (toneStack490 - (tubefilter->fConst2 * (toneStack484 + toneStack488)));
	tubefilter->toneStack493 = ((toneStack31 == 7) / toneStack478);
	float toneStack494 = (3.0621250000000006e-07f * toneStack0);
	float toneStack495 = (5.442360000000002e-07f + ((1.784904e-05f * toneStack2) + (toneStack0 * (((1.2248500000000003e-05f * toneStack2) - 5.596250000000005e-08f) - toneStack494))));
	float toneStack496 = ((9.245610000000004e-10f * toneStack2) - (2.3114025000000008e-11f * toneStack0));
	float toneStack497 = (1.0781100000000005e-09f * toneStack2);
	float toneStack498 = (2.695275000000001e-11f + (toneStack497 + (toneStack0 * (toneStack496 - 3.8387250000000005e-12f))));
	float toneStack499 = (tubefilter->fConst1 * toneStack498);
	float toneStack500 = (0.02227f * toneStack2);
	float toneStack501 = (tubefilter->fConst1 * (0.00207625f + (toneStack8 + toneStack500)));
	tubefilter->toneStack502 = ((toneStack501 + (tubefilter->fConst2 * (toneStack499 - toneStack495))) - 1.0f);
	float toneStack503 = (tubefilter->fConst3 * toneStack498);
	tubefilter->toneStack504 = ((tubefilter->fConst2 * (toneStack495 + toneStack503)) - (3 + toneStack501));
	tubefilter->toneStack505 = ((toneStack501 + (tubefilter->fConst2 * (toneStack495 - toneStack503))) - 3.0f);
	float toneStack506 = (0 - (1.0f + (toneStack501 + (tubefilter->fConst2 * (toneStack495 + toneStack499)))));
	tubefilter->toneStack507 = (1.0f / toneStack506);
	float toneStack508 = ((toneStack0 * (2.3114025000000008e-11f + toneStack496)) + (toneStack17 * ((2.695275000000001e-11f - (2.695275000000001e-11f * toneStack0)) + toneStack497)));
	float toneStack509 = (tubefilter->fConst3 * toneStack508);
	float toneStack510 = (4.6926e-08f + (((9.801000000000002e-08f * toneStack17) + (toneStack0 * (3.433375000000001e-07f - toneStack494))) + (toneStack2 * (1.8770400000000002e-06f + (1.2248500000000003e-05f * toneStack0)))));
	float toneStack511 = (0.0005567500000000001f + (toneStack500 + (toneStack8 + toneStack296)));
	float toneStack512 = (tubefilter->fConst1 * toneStack511);
	tubefilter->toneStack513 = (toneStack512 + (tubefilter->fConst2 * (toneStack510 - toneStack509)));
	float toneStack514 = (tubefilter->fConst1 * toneStack508);
	tubefilter->toneStack515 = (toneStack512 + (tubefilter->fConst2 * (toneStack514 - toneStack510)));
	float toneStack516 = (tubefilter->fConst1 * -toneStack511);
	tubefilter->toneStack517 = (toneStack516 + (tubefilter->fConst2 * (toneStack510 + toneStack509)));
	tubefilter->toneStack518 = (toneStack516 - (tubefilter->fConst2 * (toneStack510 + toneStack514)));
	tubefilter->toneStack519 = ((toneStack31 == 6) / toneStack506);
	float toneStack520 = (1.08515e-06f + ((3.108600000000001e-05f * toneStack2) + (toneStack0 * ((toneStack465 - 2.99475e-07f) - toneStack464))));
	float toneStack521 = ((1.8513000000000002e-09f * toneStack2) - (4.628250000000001e-11f * toneStack0));
	float toneStack522 = (3.3880000000000003e-09f * toneStack2);
	float toneStack523 = (8.470000000000002e-11f + (toneStack522 + (toneStack0 * (toneStack521 - 3.8417500000000006e-11f))));
	float toneStack524 = (tubefilter->fConst1 * toneStack523);
	float toneStack525 = (tubefilter->fConst1 * (toneStack472 + 0.0031515000000000002f));
	tubefilter->toneStack526 = ((toneStack525 + (tubefilter->fConst2 * (toneStack524 - toneStack520))) - 1.0f);
	float toneStack527 = (tubefilter->fConst3 * toneStack523);
	tubefilter->toneStack528 = ((tubefilter->fConst2 * (toneStack520 + toneStack527)) - (3 + toneStack525));
	tubefilter->toneStack529 = ((toneStack525 + (tubefilter->fConst2 * (toneStack520 - toneStack527))) - 3.0f);
	float toneStack530 = (-(1.0f + (toneStack525 + (tubefilter->fConst2 * (toneStack520 + toneStack524)))));
	tubefilter->toneStack531 = (1.0f / toneStack530);
	float toneStack532 = ((toneStack0 * (4.628250000000001e-11f + toneStack521)) + (toneStack17 * ((8.470000000000002e-11f - (8.470000000000002e-11f * toneStack0)) + toneStack522)));
	float toneStack533 = (tubefilter->fConst3 * toneStack532);
	float toneStack534 = (9.955000000000001e-08f + ((toneStack483 + (3.08e-07f * toneStack17)) + (toneStack2 * (toneStack482 + 3.982e-06f))));
	tubefilter->toneStack535 = (toneStack486 + (tubefilter->fConst2 * (toneStack534 - toneStack533)));
	float toneStack536 = (tubefilter->fConst1 * toneStack532);
	tubefilter->toneStack537 = (toneStack486 + (tubefilter->fConst2 * (toneStack536 - toneStack534)));
	tubefilter->toneStack538 = (toneStack490 + (tubefilter->fConst2 * (toneStack534 + toneStack533)));
	tubefilter->toneStack539 = (toneStack490 - (tubefilter->fConst2 * (toneStack534 + toneStack536)));
	tubefilter->toneStack540 = ((toneStack31 == 5) / toneStack530);
	float toneStack541 = (5.665800800000001e-07f + ((1.892924e-05f * toneStack2) + (toneStack0 * ((toneStack142 - 6.207784000000001e-08f) - toneStack141))));
	float toneStack542 = ((1.2661536800000005e-09f * toneStack2) - (2.7855380960000008e-11f * toneStack0));
	float toneStack543 = (1.6515048000000004e-09f * toneStack2);
	float toneStack544 = (3.6333105600000014e-11f + (toneStack543 + (toneStack0 * (toneStack542 - 8.477724640000006e-12f))));
	float toneStack545 = (tubefilter->fConst1 * toneStack544);
	float toneStack546 = (tubefilter->fConst1 * (toneStack150 + 0.0020497400000000004f));
	tubefilter->toneStack547 = ((toneStack546 + (tubefilter->fConst2 * (toneStack545 - toneStack541))) - 1.0f);
	float toneStack548 = (tubefilter->fConst3 * toneStack544);
	tubefilter->toneStack549 = ((tubefilter->fConst2 * (toneStack541 + toneStack548)) - (3.0f + toneStack546));
	tubefilter->toneStack550 = ((toneStack546 + (tubefilter->fConst2 * (toneStack541 - toneStack548))) - 3.0f);
	float toneStack551 = (-(1.0f + (toneStack546 + (tubefilter->fConst2 * (toneStack541 + toneStack545)))));
	tubefilter->toneStack552 = (1.0f / toneStack551);
	float toneStack553 = ((toneStack0 * (2.7855380960000008e-11f + toneStack542)) + (toneStack17 * ((3.6333105600000014e-11f - (3.6333105600000014e-11f * toneStack0)) + toneStack543)));
	float toneStack554 = (tubefilter->fConst3 * toneStack553);
	float toneStack555 = (6.505928000000001e-08f + ((toneStack161 + (1.5013680000000003e-07f * toneStack17)) + (toneStack2 * (toneStack160 + 2.95724e-06f))));
	tubefilter->toneStack556 = (toneStack164 + (tubefilter->fConst2 * (toneStack555 - toneStack554)));
	float toneStack557 = (tubefilter->fConst1 * toneStack553);
	tubefilter->toneStack558 = (toneStack164 + (tubefilter->fConst2 * (toneStack557 - toneStack555)));
	tubefilter->toneStack559 = (toneStack168 + (tubefilter->fConst2 * (toneStack555 + toneStack554)));
	tubefilter->toneStack560 = (toneStack168 - (tubefilter->fConst2 * (toneStack555 + toneStack557)));
	tubefilter->toneStack561 = ((toneStack31 == 4) / toneStack551);
	float toneStack562 = (1.0855872000000003e-07f * toneStack0);
	float toneStack563 = (3.222390000000001e-06f + (toneStack62 + (toneStack0 * (((5.6541000000000015e-06f * toneStack2) - 2.1333412800000006e-06f) - toneStack562))));
	float toneStack564 = (4.935e-10f * toneStack2);
	float toneStack565 = (toneStack564 - (9.4752e-12f * toneStack0));
	float toneStack566 = (1.41e-10f + (toneStack65 + (toneStack0 * (toneStack565 - 1.315248e-10f))));
	float toneStack567 = (tubefilter->fConst1 * toneStack566);
	float toneStack568 = (0.0002256f * toneStack0);
	float toneStack569 = (tubefilter->fConst1 * (0.015243699999999999f + (toneStack9 + toneStack568)));
	tubefilter->toneStack570 = ((toneStack569 + (tubefilter->fConst2 * (toneStack567 - toneStack563))) - 1.0f);
	float toneStack571 = (tubefilter->fConst3 * toneStack566);
	tubefilter->toneStack572 = ((tubefilter->fConst2 * (toneStack563 + toneStack571)) - (3.0f + toneStack569));
	tubefilter->toneStack573 = ((toneStack569 + (tubefilter->fConst2 * (toneStack563 - toneStack571))) - 3.0f);
	float toneStack574 = (-(1.0f + (toneStack569 + (tubefilter->fConst2 * (toneStack563 + toneStack567)))));
	tubefilter->toneStack575 = (1.0f / toneStack574);
	float toneStack576 = (1.41e-10f - (1.41e-10f * toneStack0));
	float toneStack577 = ((toneStack0 * (9.4752e-12f + toneStack565)) + (toneStack17 * (toneStack65 + toneStack576)));
	float toneStack578 = (tubefilter->fConst3 * toneStack577);
	float toneStack579 = (4.764000000000001e-08f + ((toneStack78 + (toneStack0 * (1.2265872000000003e-07f - toneStack562)))
		+ (toneStack2 * (2.48125e-06f + (5.6541000000000015e-06f * toneStack0)))));
	float toneStack580 = (0.00048120000000000004f + (toneStack9 + (toneStack21 + toneStack568)));
	float toneStack581 = (tubefilter->fConst1 * toneStack580);
	tubefilter->toneStack582 = (toneStack581 + (tubefilter->fConst2 * (toneStack579 - toneStack578)));
	float toneStack583 = (tubefilter->fConst1 * toneStack577);
	tubefilter->toneStack584 = (toneStack581 + (tubefilter->fConst2 * (toneStack583 - toneStack579)));
	float toneStack585 = (tubefilter->fConst1 * -toneStack580);
	tubefilter->toneStack586 = (toneStack585 + (tubefilter->fConst2 * (toneStack579 + toneStack578)));
	tubefilter->toneStack587 = (toneStack585 - (tubefilter->fConst2 * (toneStack579 + toneStack583)));
	tubefilter->toneStack588 = ((toneStack31 == 3) / toneStack574);
	float toneStack589 = (4.7056400000000006e-07f * toneStack0);
	float toneStack590 = (5.188640000000001e-06f + ((0.00011869100000000002f * toneStack2) + (toneStack0 * (((1.1764100000000001e-05f * toneStack2) - 4.215336e-06f) - toneStack589))));
	float toneStack591 = (toneStack564 - (1.974e-11f * toneStack0));
	float toneStack592 = (3.525e-09f * toneStack2);
	float toneStack593 = (1.41e-10f + (toneStack592 + (toneStack0 * (toneStack591 - 1.2126e-10f))));
	float toneStack594 = (tubefilter->fConst1 * toneStack593);
	float toneStack595 = (0.02503f * toneStack2);
	float toneStack596 = (tubefilter->fConst1 * (0.0157312f + (toneStack68 + toneStack595)));
	tubefilter->toneStack597 = ((toneStack596 + (tubefilter->fConst2 * (toneStack594 - toneStack590))) - 1.0f);
	float toneStack598 = (tubefilter->fConst3 * toneStack593);
	tubefilter->toneStack599 = ((tubefilter->fConst2 * (toneStack590 + toneStack598)) - (3 + toneStack596));
	tubefilter->toneStack600 = ((toneStack596 + (tubefilter->fConst2 * (toneStack590 - toneStack598))) - 3.0f);
	float toneStack601 = (-(1.0f + (toneStack596 + (tubefilter->fConst2 * (toneStack590 + toneStack594)))));
	tubefilter->toneStack602 = (1.0f / toneStack601);
	float toneStack603 = ((toneStack0 * (1.974e-11f + toneStack591)) + (toneStack17 * (toneStack576 + toneStack592)));
	float toneStack604 = (tubefilter->fConst3 * toneStack603);
	float toneStack605 = (4.764000000000001e-08f + (((4.410000000000001e-07f * toneStack17) + (toneStack0 * (4.846640000000001e-07f - toneStack589)))
		+ (toneStack2 * (1.1910000000000001e-06f + (1.1764100000000001e-05f * toneStack0)))));
	float toneStack606 = (0.0010012f + (toneStack595 + (toneStack68 + (3e-05f * toneStack17))));
	float toneStack607 = (tubefilter->fConst1 * toneStack606);
	tubefilter->toneStack608 = (toneStack607 + (tubefilter->fConst2 * (toneStack605 - toneStack604)));
	float toneStack609 = tubefilter->fConst1 * toneStack603;
	tubefilter->toneStack610 = (toneStack607 + (tubefilter->fConst2 * (toneStack609 - toneStack605)));
	float toneStack611 = tubefilter->fConst1 * -toneStack606;
	tubefilter->toneStack612 = (toneStack611 + (tubefilter->fConst2 * (toneStack605 + toneStack604)));
	tubefilter->toneStack613 = (toneStack611 - (tubefilter->fConst2 * (toneStack605 + toneStack609)));
	tubefilter->toneStack614 = ((toneStack31 == 2) / toneStack601);
	float toneStack615 = (2.9448437500000003e-06f * toneStack0);
	float toneStack616 = (1.2916875000000002e-05f + (toneStack62 + (toneStack0 * (((2.9448437500000007e-05f * toneStack2) - 8.731718750000001e-06f) - toneStack615))));
	float toneStack617 = ((2.5703125000000004e-09f * toneStack2) - (2.5703125000000003e-10f * toneStack0));
	float toneStack618 = (7.343750000000001e-10f + (toneStack65 + (toneStack0 * (toneStack617 - 4.773437500000001e-10f))));
	float toneStack619 = (tubefilter->fConst1 * toneStack618);
	float toneStack620 = (tubefilter->fConst1 * (0.01726875f + (toneStack9 + toneStack442)));
	tubefilter->toneStack621 = ((toneStack620 + (tubefilter->fConst2 * (toneStack619 - toneStack616))) - 1.0f);
	float toneStack622 = tubefilter->fConst3 * toneStack618;
	tubefilter->toneStack623 = ((tubefilter->fConst2 * (toneStack616 + toneStack622)) - (3 + toneStack620));
	tubefilter->toneStack624 = ((toneStack620 + (tubefilter->fConst2 * (toneStack616 - toneStack622))) - 3.0f);
	float toneStack625 = (-(1.0f + (toneStack620 + (tubefilter->fConst2 * (toneStack616 + toneStack619)))));
	tubefilter->toneStack626 = (1.0f / toneStack625);
	float toneStack627 = ((toneStack0 * (2.5703125000000003e-10f + toneStack617)) + (toneStack17 * (toneStack65 + (7.343750000000001e-10f - (7.343750000000001e-10f * toneStack0)))));
	float toneStack628 = (tubefilter->fConst3 * toneStack627);
	float toneStack629 = (2.48125e-07f + ((toneStack78 + (toneStack0 * (3.0182812500000004e-06f - toneStack615))) + (toneStack2 * (2.48125e-06f + (2.9448437500000007e-05f * toneStack0)))));
	float toneStack630 = (0.0025062500000000002f + (toneStack9 + toneStack454));
	float toneStack631 = (tubefilter->fConst1 * toneStack630);
	tubefilter->toneStack632 = (toneStack631 + (tubefilter->fConst2 * (toneStack629 - toneStack628)));
	float toneStack633 = tubefilter->fConst1 * toneStack627;
	tubefilter->toneStack634 = (toneStack631 + (tubefilter->fConst2 * (toneStack633 - toneStack629)));
	float toneStack635 = (tubefilter->fConst1 * -toneStack630);
	tubefilter->toneStack636 = (toneStack635 + (tubefilter->fConst2 * (toneStack629 + toneStack628)));
	tubefilter->toneStack637 = (toneStack635 - (tubefilter->fConst2 * (toneStack629 + toneStack633)));
	tubefilter->toneStack638 = ((toneStack31 == 1) / toneStack625);
	float toneStack639 = (2.5312500000000006e-07f * toneStack0);
	float toneStack640 = (7.4525e-07f + ((2.4210000000000004e-05f * toneStack2) + (toneStack0 * (((1.0125e-05f * toneStack2) - 2.75625e-07f) - toneStack639))));
	float toneStack641 = ((7.650000000000002e-10f * toneStack2) - (1.9125000000000002e-11f * toneStack0));
	float toneStack642 = (1.4000000000000001e-09f * toneStack2);
	float toneStack643 = (3.500000000000001e-11f + (toneStack642 + (toneStack0 * (toneStack641 - 1.5875000000000007e-11f))));
	float toneStack644 = (tubefilter->fConst1 * toneStack643);
	float toneStack645 = (0.02025f * toneStack2);
	float toneStack646 = (tubefilter->fConst1 * (0.0028087500000000005f + (toneStack365 + toneStack645)));
	tubefilter->toneStack647 = ((toneStack646 + (tubefilter->fConst2 * (toneStack644 - toneStack640))) - 1.0f);
	float toneStack648 = (tubefilter->fConst3 * toneStack643);
	tubefilter->toneStack649 = ((tubefilter->fConst2 * (toneStack640 + toneStack648)) - (3 + toneStack646));
	tubefilter->toneStack650 = ((toneStack646 + (tubefilter->fConst2 * (toneStack640 - toneStack648))) - 3.0f);
	float toneStack651 = (-(1.0f + (toneStack646 + (tubefilter->fConst2 * (toneStack640 + toneStack644)))));
	tubefilter->toneStack652 = 1.0f / toneStack651;
	float toneStack653 = ((toneStack0 * (1.9125000000000002e-11f + toneStack641)) + (toneStack17 * ((3.500000000000001e-11f - (3.500000000000001e-11f * toneStack0)) + toneStack642)));
	float toneStack654 = (tubefilter->fConst3 * toneStack653);
	float toneStack655 = (4.525e-08f + (((1.4e-07f * toneStack17) + (toneStack0 * (2.8437500000000003e-07f - toneStack639))) + (toneStack2 * (1.8100000000000002e-06f + (1.0125e-05f * toneStack0)))));
	float toneStack656 = (0.00050625f + (toneStack645 + (toneStack21 + toneStack365)));
	float toneStack657 = (tubefilter->fConst1 * toneStack656);
	tubefilter->toneStack658 = (toneStack657 + (tubefilter->fConst2 * (toneStack655 - toneStack654)));
	float toneStack659 = (tubefilter->fConst1 * toneStack653);
	tubefilter->toneStack660 = (toneStack657 + (tubefilter->fConst2 * (toneStack659 - toneStack655)));
	float toneStack661 = (tubefilter->fConst1 * (0 - toneStack656));
	tubefilter->toneStack662 = (toneStack661 + (tubefilter->fConst2 * (toneStack655 + toneStack654)));
	tubefilter->toneStack663 = (toneStack661 - (tubefilter->fConst2 * (toneStack655 + toneStack659)));
	tubefilter->toneStack664 = ((toneStack31 == 0) / toneStack651);
	tubefilter->v1 = tubefilter->v2 = 0;
	return 1;
}
void processTube(tubeFilter *tubefilter, float* inputs, float* outputs, unsigned frames)
{
	float tubeout;
	unsigned i, j, iMinus1;
	for (i = 0; i < frames; ++i)
	{
		//Step 1: read input sample as voltage for the source
		double ViE = inputs[i] * tubefilter->overdrived1Gain;
		tubeout = (float)advanc(&tubefilter->ckt, ViE) * tubefilter->overdrived2Gain;
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
		outputs[i] = (float)(tubefilter->pb0 * ViE + tubefilter->pb1 * tubefilter->v1 + tubefilter->pb2*tubefilter->v2);
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