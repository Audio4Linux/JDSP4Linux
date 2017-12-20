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
	updateRValues(&tubefilter->ckt, tubefilter->ci, tubefilter->ck, tubefilter->co, tubefilter->e, 0.0, tubefilter->rp, tubefilter->rg, tubefilter->ri, tubefilter->rk, tubefilter->ro, 10000.0, samplerate, insane, tubefilter->v);
	warmup_tubes(&tubefilter->ckt, warmupDuration);
	if (samplerate > 96000.0 || samplerate < 4000.0)
		return 0;
	tubefilter->fConst0 = samplerate;
	tubefilter->fConst1 = (2.0 * tubefilter->fConst0);
	tubefilter->fConst2 = pow(tubefilter->fConst1, 2.0);
	tubefilter->fConst3 = (3.0 * tubefilter->fConst1);
	tubefilter->fConst4 = (1.0691560000000003e-08 * tubefilter->fConst1);
	tubefilter->fConst5 = (3.2074680000000005e-08 * tubefilter->fConst1);
	tubefilter->fConst6 = (0.044206800000000004 * tubefilter->fConst0);
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
int InitTube(tubeFilter *tubefilter, Real *circuitparameters, Real samplerate, Real tubedrive, Real bass, Real middle, Real treble, int tonestack, int warmupDuration, int insane)
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
	tubefilter->overdrived1Gain = from_dB(tubedrive)*0.6;
	if (!activate(tubefilter, samplerate, circuitparameters, warmupDuration, insane))
		return 0;
	Real A = pow(10, 1.45);
	Real w0 = 6.283185307179586 * 1.0 / samplerate;
	Real cs = cos(w0);
	Real sn = sin(w0);
	Real AL = sn / 2.0 * sqrt((A + 1.0 / A) * (1.0 / 0.2 - 1.0) + 2.0);
	Real sq = 2.0 * sqrt(A) * AL;
	Real b0 = A*((A + 1.0) - (A - 1.0)*cs + sq);
	Real b1 = 2.0 * A*((A - 1.0) - (A + 1.0)*cs);
	Real b2 = A*((A + 1.0) - (A - 1.0)*cs - sq);
	Real a0 = (A + 1.0) + (A - 1.0)*cs + sq;
	Real a1 = -2.0 * ((A - 1.0) + (A + 1.0)*cs);
	Real a2 = (A + 1.0) + (A - 1.0)*cs - sq;
	tubefilter->pa0 = a0;
	tubefilter->pa1 = a1 / a0;
	tubefilter->pa2 = a2 / a0;
	tubefilter->pb0 = b0 / a0;
	tubefilter->pb1 = b1 / a0;
	tubefilter->pb2 = b2 / a0;
	tubefilter->overdrived2Gain = 75.0 * from_dB(30.0 - tubedrive);
	Real toneStack0 = middle / 20.0;
	Real toneStack1 = (1.3784375e-06 * toneStack0);
	Real toneStack2 = exp((3.4 * (bass / 20. - 1)));
	Real toneStack3 = (8.396625e-06 + ((7.405375e-05 * toneStack2) + (toneStack0 * (((1.3784375000000003e-05 * toneStack2) - 5.7371875e-06) - toneStack1))));
	Real toneStack4 = ((1.3062500000000001e-09 * toneStack2) - (1.30625e-10 * toneStack0));
	Real toneStack5 = (4.468750000000001e-09 * toneStack2);
	Real toneStack6 = (4.46875e-10 + (toneStack5 + (toneStack0 * (toneStack4 - 3.1625e-10))));
	Real toneStack7 = (tubefilter->fConst1 * toneStack6);
	Real toneStack8 = (0.00055 * toneStack0);
	Real toneStack9 = (0.0250625 * toneStack2);
	Real toneStack10 = (tubefilter->fConst1 * (0.01842875 + (toneStack9 + toneStack8)));
	tubefilter->toneStack11 = ((toneStack10 + (tubefilter->fConst2 * (toneStack7 - toneStack3))) - 1);
	Real toneStack12 = (tubefilter->fConst3 * toneStack6);
	tubefilter->toneStack13 = ((tubefilter->fConst2 * (toneStack3 + toneStack12)) - (3.0 + toneStack10));
	tubefilter->toneStack14 = ((toneStack10 + (tubefilter->fConst2 * (toneStack3 - toneStack12))) - 3);
	Real toneStack15 = (0 - (1 + (toneStack10 + (tubefilter->fConst2 * (toneStack3 + toneStack7)))));
	tubefilter->toneStack16 = (1.0 / toneStack15);
	Real toneStack17 = treble / 20.0;
	Real toneStack18 = ((toneStack0 * (1.30625e-10 + toneStack4)) + (toneStack17 * ((4.46875e-10 - (4.46875e-10 * toneStack0)) + toneStack5)));
	Real toneStack19 = (tubefilter->fConst3 * toneStack18);
	Real toneStack20 = (2.55375e-07 + (((9.912500000000003e-07 * toneStack17) + (toneStack0 * (1.4128125e-06 - toneStack1)))
		+ (toneStack2 * (2.5537500000000007e-06 + (1.3784375000000003e-05 * toneStack0)))));
	Real toneStack21 = (6.25e-05 * toneStack17);
	Real toneStack22 = (toneStack8 + toneStack21);
	Real toneStack23 = (0.0025062500000000002 + (toneStack9 + toneStack22));
	Real toneStack24 = (tubefilter->fConst1 * toneStack23);
	tubefilter->toneStack25 = (toneStack24 + (tubefilter->fConst2 * (toneStack20 - toneStack19)));
	Real toneStack26 = (tubefilter->fConst1 * toneStack18);
	tubefilter->toneStack27 = (toneStack24 + (tubefilter->fConst2 * (toneStack26 - toneStack20)));
	Real toneStack28 = (tubefilter->fConst1 * -toneStack23);
	tubefilter->toneStack29 = (toneStack28 + (tubefilter->fConst2 * (toneStack20 + toneStack19)));
	tubefilter->toneStack30 = (toneStack28 - (tubefilter->fConst2 * (toneStack20 + toneStack26)));
	int toneStack31 = tonestack;
	tubefilter->toneStack32 = ((toneStack31 == 23) / toneStack15);
	Real toneStack33 = (1.0607618000000002e-05 * toneStack0);
	Real toneStack34 = (3.1187760000000004e-05 + ((0.00032604000000000004 * toneStack2) + (toneStack0 * (((0.00011284700000000001 * toneStack2) - 1.9801382e-05) - toneStack33))));
	Real toneStack35 = ((3.5814000000000013e-09 * toneStack2) - (3.3665160000000007e-10 * toneStack0));
	Real toneStack36 = (8.100000000000003e-09 * toneStack2);
	Real toneStack37 = (7.614000000000002e-10 + (toneStack36 + (toneStack0 * (toneStack35 - 4.247484000000001e-10))));
	Real toneStack38 = (tubefilter->fConst1 * toneStack37);
	Real toneStack39 = (0.00188 * toneStack0);
	Real toneStack40 = (0.060025 * toneStack2);
	Real toneStack41 = (tubefilter->fConst1 * (0.027267350000000003 + (toneStack40 + toneStack39)));
	tubefilter->toneStack42 = ((toneStack41 + (tubefilter->fConst2 * (toneStack38 - toneStack34))) - 1.0);
	Real toneStack43 = (tubefilter->fConst3 * toneStack37);
	tubefilter->toneStack44 = ((tubefilter->fConst2 * (toneStack34 + toneStack43)) - (3 + toneStack41));
	tubefilter->toneStack45 = ((toneStack41 + (tubefilter->fConst2 * (toneStack34 - toneStack43))) - 3.0);
	Real toneStack46 = (0 - (1 + (toneStack41 + (tubefilter->fConst2 * (toneStack34 + toneStack38)))));
	tubefilter->toneStack47 = (1.0 / toneStack46);
	Real toneStack48 = ((toneStack0 * (3.3665160000000007e-10 + toneStack35)) + (toneStack17 * ((7.614000000000002e-10 - (7.614000000000002e-10 * toneStack0)) + toneStack36)));
	Real toneStack49 = (tubefilter->fConst3 * toneStack48);
	Real toneStack50 = (1.9176000000000002e-07 + (((5.400000000000001e-07 * toneStack17) + (toneStack0 * (1.0654618000000002e-05 - toneStack33)))
		+ (toneStack2 * (2.0400000000000004e-06 + (0.00011284700000000001 * toneStack0)))));
	Real toneStack51 = (2.5e-05 * toneStack17);
	Real toneStack52 = (0.005642350000000001 + (toneStack40 + (toneStack39 + toneStack51)));
	Real toneStack53 = (tubefilter->fConst1 * toneStack52);
	tubefilter->toneStack54 = (toneStack53 + (tubefilter->fConst2 * (toneStack50 - toneStack49)));
	Real toneStack55 = (tubefilter->fConst1 * toneStack48);
	tubefilter->toneStack56 = (toneStack53 + (tubefilter->fConst2 * (toneStack55 - toneStack50)));
	Real toneStack57 = (tubefilter->fConst1 * -toneStack52);
	tubefilter->toneStack58 = (toneStack57 + (tubefilter->fConst2 * (toneStack50 + toneStack49)));
	tubefilter->toneStack59 = (toneStack57 - (tubefilter->fConst2 * (toneStack50 + toneStack55)));
	tubefilter->toneStack60 = ((toneStack31 == 24) / toneStack46);
	Real toneStack61 = (4.7117500000000004e-07 * toneStack0);
	Real toneStack62 = (0.00011998125000000002 * toneStack2);
	Real toneStack63 = (5.718000000000001e-06 + (toneStack62 + (toneStack0 * (((1.1779375000000001e-05 * toneStack2) - 4.199450000000001e-06) - toneStack61))));
	Real toneStack64 = ((1.0281250000000001e-09 * toneStack2) - (4.1125e-11 * toneStack0));
	Real toneStack65 = (7.343750000000001e-09 * toneStack2);
	Real toneStack66 = (2.9375e-10 + (toneStack65 + (toneStack0 * (toneStack64 - 2.52625e-10))));
	Real toneStack67 = (tubefilter->fConst1 * toneStack66);
	Real toneStack68 = (0.00047000000000000004 * toneStack0);
	Real toneStack69 = (tubefilter->fConst1 * (0.015765 + (toneStack9 + toneStack68)));
	tubefilter->toneStack70 = ((toneStack69 + (tubefilter->fConst2 * (toneStack67 - toneStack63))) - 1.0);
	Real toneStack71 = (tubefilter->fConst3 * toneStack66);
	tubefilter->toneStack72 = ((tubefilter->fConst2 * (toneStack63 + toneStack71)) - (3.0 + toneStack69));
	tubefilter->toneStack73 = ((toneStack69 + (tubefilter->fConst2 * (toneStack63 - toneStack71))) - 3.0);
	Real toneStack74 = (-(1.0 + (toneStack69 + (tubefilter->fConst2 * (toneStack63 + toneStack67)))));
	tubefilter->toneStack75 = (1.0 / toneStack74);
	Real toneStack76 = ((toneStack0 * (4.1125e-11 + toneStack64)) + (toneStack17 * ((2.9375e-10 - (2.9375e-10 * toneStack0)) + toneStack65)));
	Real toneStack77 = (tubefilter->fConst3 * toneStack76);
	Real toneStack78 = (9.187500000000001e-07 * toneStack17);
	Real toneStack79 = (9.925e-08 + ((toneStack78 + (toneStack0 * (5.0055e-07 - toneStack61))) + (toneStack2 * (2.48125e-06 + (1.1779375000000001e-05 * toneStack0)))));
	Real toneStack80 = (0.0010025 + (toneStack9 + (toneStack21 + toneStack68)));
	Real toneStack81 = (tubefilter->fConst1 * toneStack80);
	tubefilter->toneStack82 = (toneStack81 + (tubefilter->fConst2 * (toneStack79 - toneStack77)));
	Real toneStack83 = (tubefilter->fConst1 * toneStack76);
	tubefilter->toneStack84 = (toneStack81 + (tubefilter->fConst2 * (toneStack83 - toneStack79)));
	Real toneStack85 = (tubefilter->fConst1 * (0 - toneStack80));
	tubefilter->toneStack86 = (toneStack85 + (tubefilter->fConst2 * (toneStack79 + toneStack77)));
	tubefilter->toneStack87 = (toneStack85 - (tubefilter->fConst2 * (toneStack79 + toneStack83)));
	tubefilter->toneStack88 = ((toneStack31 == 22) / toneStack74);
	Real toneStack89 = (3.059375000000001e-07 * toneStack0);
	Real toneStack90 = (1.5468750000000003e-06 + ((1.2718750000000003e-05 * toneStack2) + (toneStack0 * (((3.0593750000000007e-06 * toneStack2) - 8.696875000000003e-07) - toneStack89))));
	Real toneStack91 = ((2.646875e-10 * toneStack2) - (2.6468750000000002e-11 * toneStack0));
	Real toneStack92 = (7.5625e-10 * toneStack2);
	Real toneStack93 = (7.562500000000001e-11 + (toneStack92 + (toneStack0 * (toneStack91 - 4.915625000000001e-11))));
	Real toneStack94 = (tubefilter->fConst1 * toneStack93);
	Real toneStack95 = (0.005562500000000001 * toneStack2);
	Real toneStack96 = (tubefilter->fConst1 * (0.005018750000000001 + (toneStack8 + toneStack95)));
	tubefilter->toneStack97 = ((toneStack96 + (tubefilter->fConst2 * (toneStack94 - toneStack90))) - 1.0);
	Real toneStack98 = (tubefilter->fConst3 * toneStack93);
	tubefilter->toneStack99 = ((tubefilter->fConst2 * (toneStack90 + toneStack98)) - (3 + toneStack96));
	tubefilter->toneStack100 = ((toneStack96 + (tubefilter->fConst2 * (toneStack90 - toneStack98))) - 3.0);
	Real toneStack101 = (-(1 + (toneStack96 + (tubefilter->fConst2 * (toneStack90 + toneStack94)))));
	tubefilter->toneStack102 = (1.0 / toneStack101);
	Real toneStack103 = ((toneStack0 * (2.6468750000000002e-11 + toneStack91)) + (toneStack17 * ((7.562500000000001e-11 - (7.562500000000001e-11 * toneStack0)) + toneStack92)));
	Real toneStack104 = (tubefilter->fConst3 * toneStack103);
	Real toneStack105 = (6.1875e-08 + (((2.75e-07 * toneStack17) + (toneStack0 * (3.403125000000001e-07 - toneStack89))) + (toneStack2 * (6.1875e-07 + (3.0593750000000007e-06 * toneStack0)))));
	Real toneStack106 = (0.00055625 + (toneStack22 + toneStack95));
	Real toneStack107 = (tubefilter->fConst1 * toneStack106);
	Real toneStack109 = (tubefilter->fConst1 * toneStack103);
	tubefilter->toneStack110 = (toneStack107 + (tubefilter->fConst2 * (toneStack109 - toneStack105)));
	Real toneStack111 = (tubefilter->fConst1 * -toneStack106);
	tubefilter->toneStack112 = (toneStack111 + (tubefilter->fConst2 * (toneStack105 + toneStack104)));
	tubefilter->toneStack113 = (toneStack111 - (tubefilter->fConst2 * (toneStack105 + toneStack109)));
	tubefilter->toneStack114 = ((toneStack31 == 21) / toneStack101);
	Real toneStack115 = (2.2193400000000003e-07 * toneStack0);
	Real toneStack116 = (2.7073879999999998e-06 + ((4.9553415999999996e-05 * toneStack2) + (toneStack0 * (((4.882548000000001e-06 * toneStack2) - 1.964318e-06) - toneStack115))));
	Real toneStack117 = ((3.4212992000000004e-10 * toneStack2) - (1.5551360000000004e-11 * toneStack0));
	Real toneStack118 = (2.3521432000000003e-09 * toneStack2);
	Real toneStack119 = (1.0691560000000001e-10 + (toneStack118 + (toneStack0 * (toneStack117 - 9.136424e-11))));
	Real toneStack120 = (tubefilter->fConst1 * toneStack119);
	Real toneStack121 = (0.0103884 * toneStack2);
	Real toneStack122 = (tubefilter->fConst1 * (0.009920600000000002 + (toneStack68 + toneStack121)));
	tubefilter->toneStack123 = ((toneStack122 + (tubefilter->fConst2 * (toneStack120 - toneStack116))) - 1.0);
	Real toneStack124 = (tubefilter->fConst3 * toneStack119);
	tubefilter->toneStack125 = ((tubefilter->fConst2 * (toneStack116 + toneStack124)) - (3 + toneStack122));
	tubefilter->toneStack126 = ((toneStack122 + (tubefilter->fConst2 * (toneStack116 - toneStack124))) - 3.0);
	Real toneStack127 = -(1.0 + (toneStack122 + (tubefilter->fConst2 * (toneStack116 + toneStack120))));
	tubefilter->toneStack128 = 1.0 / toneStack127;
	Real toneStack129 = ((toneStack0 * (1.5551360000000004e-11 + toneStack117)) + (toneStack17 * ((1.0691560000000001e-10 - (1.0691560000000001e-10 * toneStack0)) + toneStack118)));
	Real toneStack130 = (tubefilter->fConst3 * toneStack129);
	Real toneStack131 = (4.3428e-08 + (((4.5496e-07 * toneStack17) + (toneStack0 * (2.4468200000000005e-07 - toneStack115))) + (toneStack2 * (9.55416e-07 + (4.882548000000001e-06 * toneStack0)))));
	Real toneStack132 = (0.00047220000000000004 + (toneStack121 + (toneStack68 + (4.84e-05 * toneStack17))));
	Real toneStack133 = (tubefilter->fConst1 * toneStack132);
	tubefilter->toneStack134 = (toneStack133 + (tubefilter->fConst2 * (toneStack131 - toneStack130)));
	Real toneStack135 = (tubefilter->fConst1 * toneStack129);
	tubefilter->toneStack136 = (toneStack133 + (tubefilter->fConst2 * (toneStack135 - toneStack131)));
	Real toneStack137 = (tubefilter->fConst1 * -toneStack132);
	tubefilter->toneStack138 = (toneStack137 + (tubefilter->fConst2 * (toneStack131 + toneStack130)));
	tubefilter->toneStack139 = (toneStack137 - (tubefilter->fConst2 * (toneStack131 + toneStack135)));
	tubefilter->toneStack140 = ((toneStack31 == 20) / toneStack127);
	Real toneStack141 = (2.3926056000000006e-07 * toneStack0);
	Real toneStack142 = (1.0875480000000001e-05 * toneStack2);
	Real toneStack143 = (1.1144196800000003e-06 + ((3.659304000000001e-05 * toneStack2) + (toneStack0 * ((toneStack142 - 4.347578400000001e-07) - toneStack141))));
	Real toneStack144 = ((1.4413132800000006e-09 * toneStack2) - (3.1708892160000014e-11 * toneStack0));
	Real toneStack145 = (3.403100800000001e-09 * toneStack2);
	Real toneStack146 = (7.486821760000003e-11 + (toneStack145 + (toneStack0 * (toneStack144 - 4.315932544000001e-11))));
	Real toneStack147 = (tubefilter->fConst1 * toneStack146);
	Real toneStack148 = (0.00048400000000000006 * toneStack0);
	Real toneStack149 = (0.022470000000000004 * toneStack2);
	Real toneStack150 = (toneStack149 + toneStack148);
	Real toneStack151 = (tubefilter->fConst1 * (0.00358974 + toneStack150));
	tubefilter->toneStack152 = ((toneStack151 + (tubefilter->fConst2 * (toneStack147 - toneStack143))) - 1);
	Real toneStack153 = (tubefilter->fConst3 * toneStack146);
	tubefilter->toneStack154 = ((tubefilter->fConst2 * (toneStack143 + toneStack153)) - (3 + toneStack151));
	tubefilter->toneStack155 = ((toneStack151 + (tubefilter->fConst2 * (toneStack143 - toneStack153))) - 3);
	Real toneStack156 = (0 - (1 + (toneStack151 + (tubefilter->fConst2 * (toneStack143 + toneStack147)))));
	tubefilter->toneStack157 = (1.0 / toneStack156);
	Real toneStack158 = ((toneStack0 * (3.1708892160000014e-11 + toneStack144)) + (toneStack17 * ((7.486821760000003e-11 - (7.486821760000003e-11 * toneStack0)) + toneStack145)));
	Real toneStack159 = (tubefilter->fConst3 * toneStack158);
	Real toneStack160 = (1.0875480000000001e-05 * toneStack0);
	Real toneStack161 = (toneStack0 * (2.893061600000001e-07 - toneStack141));
	Real toneStack162 = (8.098288000000002e-08 + (((3.0937280000000007e-07 * toneStack17) + toneStack161) + (toneStack2 * (3.6810400000000007e-06 + toneStack160))));
	Real toneStack163 = (0.00049434 + (toneStack149 + (toneStack148 + (0.0001034 * toneStack17))));
	Real toneStack164 = (tubefilter->fConst1 * toneStack163);
	tubefilter->toneStack165 = (toneStack164 + (tubefilter->fConst2 * (toneStack162 - toneStack159)));
	Real toneStack166 = (tubefilter->fConst1 * toneStack158);
	tubefilter->toneStack167 = (toneStack164 + (tubefilter->fConst2 * (toneStack166 - toneStack162)));
	Real toneStack168 = (tubefilter->fConst1 * -toneStack163);
	tubefilter->toneStack169 = (toneStack168 + (tubefilter->fConst2 * (toneStack162 + toneStack159)));
	tubefilter->toneStack170 = (toneStack168 - (tubefilter->fConst2 * (toneStack162 + toneStack166)));
	tubefilter->toneStack171 = ((toneStack31 == 19) / toneStack156);
	Real toneStack172 = (7.790052600000002e-07 * toneStack0);
	Real toneStack173 = (1.4106061200000003e-06 + ((3.7475640000000014e-05 * toneStack2) + (toneStack0 * (((2.3606220000000006e-05 * toneStack2) - 3.2220474e-07) - toneStack172))));
	Real toneStack174 = ((1.5406083e-09 * toneStack2) - (5.08400739e-11 * toneStack0));
	Real toneStack175 = (1.9775250000000004e-09 * toneStack2);
	Real toneStack176 = (6.5258325e-11 + (toneStack175 + (toneStack0 * (toneStack174 - 1.4418251099999996e-11))));
	Real toneStack177 = (tubefilter->fConst1 * toneStack176);
	Real toneStack178 = (0.001551 * toneStack0);
	Real toneStack179 = (0.015220000000000001 * toneStack2);
	Real toneStack180 = (tubefilter->fConst1 * (0.0037192600000000003 + (toneStack179 + toneStack178)));
	tubefilter->toneStack181 = ((toneStack180 + (tubefilter->fConst2 * (toneStack177 - toneStack173))) - 1);
	Real toneStack182 = (tubefilter->fConst3 * toneStack176);
	tubefilter->toneStack183 = ((tubefilter->fConst2 * (toneStack173 + toneStack182)) - (3 + toneStack180));
	tubefilter->toneStack184 = ((toneStack180 + (tubefilter->fConst2 * (toneStack173 - toneStack182))) - 3);
	Real toneStack185 = (0 - (1 + (toneStack180 + (tubefilter->fConst2 * (toneStack173 + toneStack177)))));
	tubefilter->toneStack186 = (1.0 / toneStack185);
	Real toneStack187 = ((toneStack0 * (5.08400739e-11 + toneStack174)) + (toneStack17 * ((6.5258325e-11 - (6.5258325e-11 * toneStack0)) + toneStack175)));
	Real toneStack188 = (tubefilter->fConst3 * toneStack187);
	Real toneStack189 = (5.018112e-08 + (((1.7391e-07 * toneStack17) + (toneStack0 * (8.643102600000002e-07 - toneStack172)))
		+ (toneStack2 * (1.5206400000000001e-06 + (2.3606220000000006e-05 * toneStack0)))));
	Real toneStack190 = (0.0005022600000000001 + (toneStack179 + (toneStack178 + (5.4999999999999995e-05 * toneStack17))));
	Real toneStack191 = (tubefilter->fConst1 * toneStack190);
	tubefilter->toneStack192 = (toneStack191 + (tubefilter->fConst2 * (toneStack189 - toneStack188)));
	Real toneStack193 = (tubefilter->fConst1 * toneStack187);
	tubefilter->toneStack194 = (toneStack191 + (tubefilter->fConst2 * (toneStack193 - toneStack189)));
	Real toneStack195 = (tubefilter->fConst1 * -toneStack190);
	tubefilter->toneStack196 = (toneStack195 + (tubefilter->fConst2 * (toneStack189 + toneStack188)));
	tubefilter->toneStack197 = (toneStack195 - (tubefilter->fConst2 * (toneStack189 + toneStack193)));
	tubefilter->toneStack198 = ((toneStack31 == 18) / toneStack185);
	Real toneStack199 = (4.7047000000000006e-07 * toneStack0);
	Real toneStack200 = (5.107200000000001e-06 + ((0.00011849250000000002 * toneStack2) + (toneStack0 * (((1.1761750000000001e-05 * toneStack2) - 4.217780000000001e-06) - toneStack199))));
	Real toneStack201 = ((4.1125e-10 * toneStack2) - (1.645e-11 * toneStack0));
	Real toneStack202 = (2.9375000000000002e-09 * toneStack2);
	Real toneStack203 = (1.175e-10 + (toneStack202 + (toneStack0 * (toneStack201 - 1.0105e-10))));
	Real toneStack204 = (tubefilter->fConst1 * toneStack203);
	Real toneStack205 = (0.025025000000000002 * toneStack2);
	Real toneStack206 = (tubefilter->fConst1 * (0.015726 + (toneStack68 + toneStack205)));
	tubefilter->toneStack207 = ((toneStack206 + (tubefilter->fConst2 * (toneStack204 - toneStack200))) - 1);
	Real toneStack208 = (tubefilter->fConst3 * toneStack203);
	tubefilter->toneStack209 = ((tubefilter->fConst2 * (toneStack200 + toneStack208)) - (3 + toneStack206));
	tubefilter->toneStack210 = ((toneStack206 + (tubefilter->fConst2 * (toneStack200 - toneStack208))) - 3);
	Real toneStack211 = (0 - (1 + (toneStack206 + (tubefilter->fConst2 * (toneStack200 + toneStack204)))));
	tubefilter->toneStack212 = (1.0 / toneStack211);
	Real toneStack213 = ((toneStack0 * (1.645e-11 + toneStack201)) + (toneStack17 * ((1.175e-10 - (1.175e-10 * toneStack0)) + toneStack202)));
	Real toneStack214 = (tubefilter->fConst3 * toneStack213);
	Real toneStack215 = (3.9700000000000005e-08 + (((3.675000000000001e-07 * toneStack17) + (toneStack0 * (4.8222e-07 - toneStack199)))
		+ (toneStack2 * (9.925e-07 + (1.1761750000000001e-05 * toneStack0)))));
	Real toneStack216 = (0.001001 + (toneStack205 + (toneStack51 + toneStack68)));
	Real toneStack217 = (tubefilter->fConst1 * toneStack216);
	tubefilter->toneStack218 = (toneStack217 + (tubefilter->fConst2 * (toneStack215 - toneStack214)));
	Real toneStack219 = (tubefilter->fConst1 * toneStack213);
	tubefilter->toneStack220 = (toneStack217 + (tubefilter->fConst2 * (toneStack219 - toneStack215)));
	Real toneStack221 = (tubefilter->fConst1 * (0 - toneStack216));
	tubefilter->toneStack222 = (toneStack221 + (tubefilter->fConst2 * (toneStack215 + toneStack214)));
	tubefilter->toneStack223 = (toneStack221 - (tubefilter->fConst2 * (toneStack215 + toneStack219)));
	tubefilter->toneStack224 = ((toneStack31 == 17) / toneStack211);
	Real toneStack225 = (3.0896250000000005e-07 * toneStack0);
	Real toneStack226 = (6.338090000000001e-07 + ((1.8734760000000003e-05 * toneStack2) + (toneStack0 * (((1.2358500000000002e-05 * toneStack2) - 1.361249999999999e-08) - toneStack225))));
	Real toneStack227 = ((1.6037340000000005e-09 * toneStack2) - (4.0093350000000015e-11 * toneStack0));
	Real toneStack228 = (1.8198400000000004e-09 * toneStack2);
	Real toneStack229 = (4.5496000000000015e-11 + (toneStack228 + (toneStack0 * (toneStack227 - 5.40265e-12))));
	Real toneStack230 = (tubefilter->fConst1 * toneStack229);
	Real toneStack231 = (tubefilter->fConst1 * (0.00208725 + (toneStack8 + toneStack149)));
	tubefilter->toneStack232 = ((toneStack231 + (tubefilter->fConst2 * (toneStack230 - toneStack226))) - 1);
	Real toneStack233 = (tubefilter->fConst3 * toneStack229);
	tubefilter->toneStack234 = ((tubefilter->fConst2 * (toneStack226 + toneStack233)) - (3 + toneStack231));
	tubefilter->toneStack235 = ((toneStack231 + (tubefilter->fConst2 * (toneStack226 - toneStack233))) - 3);
	Real toneStack236 = (0 - (1 + (toneStack231 + (tubefilter->fConst2 * (toneStack226 + toneStack230)))));
	tubefilter->toneStack237 = (1.0 / toneStack236);
	Real toneStack238 = ((toneStack0 * (4.0093350000000015e-11 + toneStack227)) + (toneStack17 * ((4.5496000000000015e-11 - (4.5496000000000015e-11 * toneStack0)) + toneStack228)));
	Real toneStack239 = (tubefilter->fConst3 * toneStack238);
	Real toneStack240 = (8.1169e-08 + (((1.6544000000000003e-07 * toneStack17) + (toneStack0 * (3.735875000000001e-07 - toneStack225))) + (toneStack2 * (3.24676e-06 + (1.2358500000000002e-05 * toneStack0)))));
	Real toneStack241 = (0.00011750000000000001 * toneStack17);
	Real toneStack242 = (0.0005617500000000001 + (toneStack149 + (toneStack8 + toneStack241)));
	Real toneStack243 = (tubefilter->fConst1 * toneStack242);
	Real toneStack245 = (tubefilter->fConst1 * toneStack238);
	tubefilter->toneStack246 = (toneStack243 + (tubefilter->fConst2 * (toneStack245 - toneStack240)));
	Real toneStack247 = (tubefilter->fConst1 * -toneStack242);
	tubefilter->toneStack248 = (toneStack247 + (tubefilter->fConst2 * (toneStack240 + toneStack239)));
	tubefilter->toneStack249 = (toneStack247 - (tubefilter->fConst2 * (toneStack240 + toneStack245)));
	tubefilter->toneStack250 = ((toneStack31 == 16) / toneStack236);
	Real toneStack251 = (2.7256800000000006e-07 * toneStack0);
	Real toneStack252 = (1.4234760000000002e-06 + ((2.851440000000001e-05 * toneStack2) + (toneStack0 * (((6.8142000000000025e-06 * toneStack2) - 7.876920000000001e-07) - toneStack251))));
	Real toneStack253 = ((4.724676000000001e-10 * toneStack2) - (1.8898704000000002e-11 * toneStack0));
	Real toneStack254 = (1.6641900000000002e-09 * toneStack2);
	Real toneStack255 = (6.656760000000001e-11 + (toneStack254 + (toneStack0 * (toneStack253 - 4.7668896000000004e-11))));
	Real toneStack256 = (tubefilter->fConst1 * toneStack255);
	Real toneStack257 = (0.0008200000000000001 * toneStack0);
	Real toneStack258 = (0.00831 * toneStack2);
	Real toneStack259 = (tubefilter->fConst1 * (0.005107400000000001 + (toneStack258 + toneStack257)));
	tubefilter->toneStack260 = ((toneStack259 + (tubefilter->fConst2 * (toneStack256 - toneStack252))) - 1);
	Real toneStack261 = (tubefilter->fConst3 * toneStack255);
	tubefilter->toneStack262 = ((tubefilter->fConst2 * (toneStack252 + toneStack261)) - (3 + toneStack259));
	tubefilter->toneStack263 = ((toneStack259 + (tubefilter->fConst2 * (toneStack252 - toneStack261))) - 3);
	Real toneStack264 = (0 - (1 + (toneStack259 + (tubefilter->fConst2 * (toneStack252 + toneStack256)))));
	tubefilter->toneStack265 = (1.0 / toneStack264);
	Real toneStack266 = ((toneStack0 * (1.8898704000000002e-11 + toneStack253)) + (toneStack17 * ((6.656760000000001e-11 - (6.656760000000001e-11 * toneStack0)) + toneStack254)));
	Real toneStack267 = (tubefilter->fConst3 * toneStack266);
	Real toneStack268 = (3.1116000000000005e-08 + (((2.829e-07 * toneStack17) + (toneStack0 * (3.2176800000000005e-07 - toneStack251))) + (toneStack2 * (7.779000000000002e-07 + (6.8142000000000025e-06 * toneStack0)))));
	Real toneStack269 = (0.00033240000000000006 + (toneStack258 + (toneStack257 + (6e-05 * toneStack17))));
	Real toneStack270 = (tubefilter->fConst1 * toneStack269);
	tubefilter->toneStack271 = (toneStack270 + (tubefilter->fConst2 * (toneStack268 - toneStack267)));
	Real toneStack272 = (tubefilter->fConst1 * toneStack266);
	tubefilter->toneStack273 = (toneStack270 + (tubefilter->fConst2 * (toneStack272 - toneStack268)));
	Real toneStack274 = (tubefilter->fConst1 * -toneStack269);
	tubefilter->toneStack275 = (toneStack274 + (tubefilter->fConst2 * (toneStack268 + toneStack267)));
	tubefilter->toneStack276 = (toneStack274 - (tubefilter->fConst2 * (toneStack268 + toneStack272)));
	tubefilter->toneStack277 = ((toneStack31 == 15) / toneStack264);
	Real toneStack278 = (4.0108000000000004e-07 * toneStack0);
	Real toneStack279 = (5.050300000000001e-06 + ((0.00010263250000000001 * toneStack2) + (toneStack0 * (((1.0027e-05 * toneStack2) - 3.5719200000000006e-06) - toneStack278))));
	Real toneStack280 = ((9.45e-10 * toneStack2) - (3.78e-11 * toneStack0));
	Real toneStack281 = (6.75e-09 * toneStack2);
	Real toneStack282 = (2.7e-10 + (toneStack281 + (toneStack0 * (toneStack280 - 2.3219999999999998e-10))));
	Real toneStack283 = (tubefilter->fConst1 * toneStack282);
	Real toneStack284 = (0.0004 * toneStack0);
	Real toneStack285 = (0.025067500000000003 * toneStack2);
	Real toneStack286 = (tubefilter->fConst1 * (0.0150702 + (toneStack285 + toneStack284)));
	tubefilter->toneStack287 = ((toneStack286 + (tubefilter->fConst2 * (toneStack283 - toneStack279))) - 1);
	Real toneStack288 = (tubefilter->fConst3 * toneStack282);
	tubefilter->toneStack289 = ((tubefilter->fConst2 * (toneStack279 + toneStack288)) - (3 + toneStack286));
	tubefilter->toneStack290 = ((toneStack286 + (tubefilter->fConst2 * (toneStack279 - toneStack288))) - 3);
	Real toneStack291 = (0 - (1 + (toneStack286 + (tubefilter->fConst2 * (toneStack279 + toneStack283)))));
	tubefilter->toneStack292 = (1.0 / toneStack291);
	Real toneStack293 = ((toneStack0 * (3.78e-11 + toneStack280)) + (toneStack17 * ((2.7e-10 - (2.7e-10 * toneStack0)) + toneStack281)));
	Real toneStack294 = (tubefilter->fConst3 * toneStack293);
	Real toneStack295 = (1.0530000000000001e-07 + (((9.45e-07 * toneStack17) + (toneStack0 * (4.2808000000000006e-07 - toneStack278))) + (toneStack2 * (2.6324999999999998e-06 + (1.0027e-05 * toneStack0)))));
	Real toneStack296 = (6.75e-05 * toneStack17);
	Real toneStack297 = (0.0010027 + (toneStack285 + (toneStack284 + toneStack296)));
	Real toneStack298 = (tubefilter->fConst1 * toneStack297);
	tubefilter->toneStack299 = (toneStack298 + (tubefilter->fConst2 * (toneStack295 - toneStack294)));
	Real toneStack300 = (tubefilter->fConst1 * toneStack293);
	tubefilter->toneStack301 = (toneStack298 + (tubefilter->fConst2 * (toneStack300 - toneStack295)));
	Real toneStack302 = (tubefilter->fConst1 * -toneStack297);
	tubefilter->toneStack303 = (toneStack302 + (tubefilter->fConst2 * (toneStack295 + toneStack294)));
	tubefilter->toneStack304 = (toneStack302 - (tubefilter->fConst2 * (toneStack295 + toneStack300)));
	tubefilter->toneStack305 = ((toneStack31 == 14) / toneStack291);
	Real toneStack306 = (1.95976e-07 * toneStack0);
	Real toneStack307 = (9.060568000000001e-07 + ((8.801210000000002e-06 * toneStack2) + (toneStack0 * (((2.4497000000000004e-06 * toneStack2) - 4.3256399999999996e-07) - toneStack306))));
	Real toneStack308 = ((2.0778120000000008e-10 * toneStack2) - (1.6622496000000003e-11 * toneStack0));
	Real toneStack309 = (5.553900000000002e-10 * toneStack2);
	Real toneStack310 = (4.4431200000000016e-11 + (toneStack309 + (toneStack0 * (toneStack308 - 2.7808704000000013e-11))));
	Real toneStack311 = (tubefilter->fConst1 * toneStack310);
	Real toneStack312 = (0.00044 * toneStack0);
	Real toneStack313 = (0.0055675 * toneStack2);
	Real toneStack314 = (tubefilter->fConst1 * (0.0035049 + (toneStack313 + toneStack312)));
	tubefilter->toneStack315 = ((toneStack314 + (tubefilter->fConst2 * (toneStack311 - toneStack307))) - 1);
	Real toneStack316 = (tubefilter->fConst3 * toneStack310);
	tubefilter->toneStack317 = ((tubefilter->fConst2 * (toneStack307 + toneStack316)) - (3 + toneStack314));
	tubefilter->toneStack318 = ((toneStack314 + (tubefilter->fConst2 * (toneStack307 - toneStack316))) - 3);
	Real toneStack319 = (0 - (1 + (toneStack314 + (tubefilter->fConst2 * (toneStack307 + toneStack311)))));
	tubefilter->toneStack320 = (1.0 / toneStack319);
	Real toneStack321 = ((toneStack0 * (1.6622496000000003e-11 + toneStack308)) + (toneStack17 * ((4.4431200000000016e-11 - (4.4431200000000016e-11 * toneStack0)) + toneStack309)));
	Real toneStack322 = (tubefilter->fConst3 * toneStack321);
	Real toneStack323 = (4.585680000000001e-08 + (((2.0196000000000004e-07 * toneStack17) + (toneStack0 * (2.2567600000000002e-07 - toneStack306))) + (toneStack2 * (5.732100000000001e-07 + (2.4497000000000004e-06 * toneStack0)))));
	Real toneStack324 = (0.00044540000000000004 + (toneStack313 + (toneStack296 + toneStack312)));
	Real toneStack325 = (tubefilter->fConst1 * toneStack324);
	tubefilter->toneStack326 = (toneStack325 + (tubefilter->fConst2 * (toneStack323 - toneStack322)));
	Real toneStack327 = (tubefilter->fConst1 * toneStack321);
	tubefilter->toneStack328 = (toneStack325 + (tubefilter->fConst2 * (toneStack327 - toneStack323)));
	Real toneStack329 = (tubefilter->fConst1 * (0 - toneStack324));
	tubefilter->toneStack330 = (toneStack329 + (tubefilter->fConst2 * (toneStack323 + toneStack322)));
	tubefilter->toneStack331 = (toneStack329 - (tubefilter->fConst2 * (toneStack323 + toneStack327)));
	tubefilter->toneStack332 = ((toneStack31 == 13) / toneStack319);
	Real toneStack333 = (4.9434000000000004e-08 * toneStack0);
	Real toneStack334 = (7.748796000000001e-07 + ((2.8889960000000004e-05 * toneStack2) + (toneStack0 * (((4.943400000000001e-06 * toneStack2) - 1.2634599999999999e-07) - toneStack333))));
	Real toneStack335 = ((1.2443156000000004e-09 * toneStack2) - (1.2443156000000002e-11 * toneStack0));
	Real toneStack336 = (5.345780000000001e-09 * toneStack2);
	Real toneStack337 = (5.345780000000001e-11 + (toneStack336 + (toneStack0 * (toneStack335 - 4.101464400000001e-11))));
	Real toneStack338 = (tubefilter->fConst1 * toneStack337);
	Real toneStack339 = (0.00022 * toneStack0);
	Real toneStack340 = (tubefilter->fConst1 * (0.0025277 + (toneStack149 + toneStack339)));
	tubefilter->toneStack341 = ((toneStack340 + (tubefilter->fConst2 * (toneStack338 - toneStack334))) - 1.0);
	Real toneStack342 = (tubefilter->fConst3 * toneStack337);
	tubefilter->toneStack343 = ((tubefilter->fConst2 * (toneStack334 + toneStack342)) - (3 + toneStack340));
	tubefilter->toneStack344 = ((toneStack340 + (tubefilter->fConst2 * (toneStack334 - toneStack342))) - 3.0);
	Real toneStack345 = (0 - (1 + (toneStack340 + (tubefilter->fConst2 * (toneStack334 + toneStack338)))));
	tubefilter->toneStack346 = (1.0 / toneStack345);
	Real toneStack347 = ((toneStack0 * (1.2443156000000002e-11 + toneStack335)) + (toneStack17 * ((5.345780000000001e-11 - (5.345780000000001e-11 * toneStack0)) + toneStack336)));
	Real toneStack348 = (tubefilter->fConst3 * toneStack347);
	Real toneStack349 = (6.141960000000001e-08 + (((4.859800000000001e-07 * toneStack17) + (toneStack0 * (1.0113400000000001e-07 - toneStack333))) + (toneStack2 * (6.141960000000001e-06
		+ (4.943400000000001e-06 * toneStack0)))));
	Real toneStack350 = (0.00022470000000000001 + (toneStack149 + (toneStack339 + (0.00023500000000000002 * toneStack17))));
	Real toneStack351 = (tubefilter->fConst1 * toneStack350);
	tubefilter->toneStack352 = (toneStack351 + (tubefilter->fConst2 * (toneStack349 - toneStack348)));
	Real toneStack353 = (tubefilter->fConst1 * toneStack347);
	tubefilter->toneStack354 = (toneStack351 + (tubefilter->fConst2 * (toneStack353 - toneStack349)));
	Real toneStack355 = (tubefilter->fConst1 * (0 - toneStack350));
	tubefilter->toneStack356 = (toneStack355 + (tubefilter->fConst2 * (toneStack349 + toneStack348)));
	tubefilter->toneStack357 = (toneStack355 - (tubefilter->fConst2 * (toneStack349 + toneStack353)));
	tubefilter->toneStack358 = ((toneStack31 == 12) / toneStack345);
	Real toneStack359 = (2.5587500000000006e-07 * toneStack0);
	Real toneStack360 = (7.717400000000001e-07 + ((2.2033600000000005e-05 * toneStack2) + (toneStack0 * (((1.0235000000000001e-05 * toneStack2) - 1.5537499999999997e-07) - toneStack359))));
	Real toneStack361 = ((1.3959000000000001e-09 * toneStack2) - (3.48975e-11 * toneStack0));
	Real toneStack362 = (2.2090000000000005e-09 * toneStack2);
	Real toneStack363 = (5.522500000000001e-11 + (toneStack362 + (toneStack0 * (toneStack361 - 2.0327500000000007e-11))));
	Real toneStack364 = (tubefilter->fConst1 * toneStack363);
	Real toneStack365 = (0.0005 * toneStack0);
	Real toneStack366 = (0.020470000000000002 * toneStack2);
	Real toneStack367 = (tubefilter->fConst1 * (0.0025092499999999998 + (toneStack366 + toneStack365)));
	tubefilter->toneStack368 = ((toneStack367 + (tubefilter->fConst2 * (toneStack364 - toneStack360))) - 1);
	Real toneStack369 = (tubefilter->fConst3 * toneStack363);
	tubefilter->toneStack370 = ((tubefilter->fConst2 * (toneStack360 + toneStack369)) - (3 + toneStack367));
	tubefilter->toneStack371 = ((toneStack367 + (tubefilter->fConst2 * (toneStack360 - toneStack369))) - 3);
	Real toneStack372 = (-(1.0 + (toneStack367 + (tubefilter->fConst2 * (toneStack360 + toneStack364)))));
	tubefilter->toneStack373 = (1.0 / toneStack372);
	Real toneStack374 = ((toneStack0 * (3.48975e-11 + toneStack361)) + (toneStack17 * ((5.522500000000001e-11 - (5.522500000000001e-11 * toneStack0)) + toneStack362)));
	Real toneStack375 = (tubefilter->fConst3 * toneStack374);
	Real toneStack376 = (8.084000000000001e-08 + (((2.2090000000000003e-07 * toneStack17) + (toneStack0 * (3.146250000000001e-07 - toneStack359)))
		+ (toneStack2 * (3.2336000000000007e-06 + (1.0235000000000001e-05 * toneStack0)))));
	Real toneStack377 = (0.00051175 + (toneStack366 + (toneStack241 + toneStack365)));
	Real toneStack378 = (tubefilter->fConst1 * toneStack377);
	tubefilter->toneStack379 = (toneStack378 + (tubefilter->fConst2 * (toneStack376 - toneStack375)));
	Real toneStack380 = (tubefilter->fConst1 * toneStack374);
	tubefilter->toneStack381 = (toneStack378 + (tubefilter->fConst2 * (toneStack380 - toneStack376)));
	Real toneStack382 = (tubefilter->fConst1 * -toneStack377);
	tubefilter->toneStack383 = (toneStack382 + (tubefilter->fConst2 * (toneStack376 + toneStack375)));
	tubefilter->toneStack384 = (toneStack382 - (tubefilter->fConst2 * (toneStack376 + toneStack380)));
	tubefilter->toneStack385 = ((toneStack31 == 11) / toneStack372);
	Real toneStack386 = (0.00022854915600000004 * toneStack0);
	Real toneStack387 = (0.00010871476000000002 + ((0.00010719478000000002 * toneStack2) + (toneStack0 * ((0.00012621831200000002 + (0.00022854915600000004 * toneStack2)) - toneStack386))));
	Real toneStack388 = ((3.421299200000001e-08 * toneStack2) - (3.421299200000001e-08 * toneStack0));
	Real toneStack389 = (1.0 + (toneStack2 + (93531720.34763868 * (toneStack0 * (2.3521432000000005e-08 + toneStack388)))));
	Real toneStack390 = (tubefilter->fConst4 * toneStack389);
	Real toneStack391 = (tubefilter->fConst1 * (0.036906800000000003 + ((0.022103400000000002 * toneStack2) + (0.01034 * toneStack0))));
	tubefilter->toneStack392 = ((toneStack391 + (tubefilter->fConst2 * (toneStack390 - toneStack387))) - 1.0);
	Real toneStack393 = (tubefilter->fConst5 * toneStack389);
	tubefilter->toneStack394 = ((tubefilter->fConst2 * (toneStack387 + toneStack393)) - (3.0 + toneStack391));
	tubefilter->toneStack395 = ((toneStack391 + (tubefilter->fConst2 * (toneStack387 - toneStack393))) - 3.0);
	Real toneStack396 = ((tubefilter->fConst2 * (-(toneStack387 + toneStack390))) - (1.0 + toneStack391));
	tubefilter->toneStack397 = (1.0 / toneStack396);
	Real toneStack398 = ((toneStack0 * (3.421299200000001e-08 + toneStack388)) + (toneStack17 * ((1.0691560000000003e-08 - (1.0691560000000003e-08 * toneStack0)) + (1.0691560000000003e-08 * toneStack2))));
	Real toneStack399 = (tubefilter->fConst3 * toneStack398);
	Real toneStack400 = (3.7947800000000004e-06 + (((1.5199800000000001e-06 * toneStack17) + (toneStack0 * (0.00022961831200000004 - toneStack386))) + (toneStack2 * (3.7947800000000004e-06 + toneStack386))));
	Real toneStack401 = (1.0 + (toneStack2 + ((0.0046780133373146215 * toneStack17) + (0.4678013337314621 * toneStack0))));
	Real toneStack402 = (tubefilter->fConst6 * toneStack401);
	tubefilter->toneStack403 = (toneStack402 + (tubefilter->fConst2 * (toneStack400 - toneStack399)));
	Real toneStack404 = (tubefilter->fConst1 * toneStack398);
	tubefilter->toneStack405 = (toneStack402 + (tubefilter->fConst2 * (toneStack404 - toneStack400)));
	Real toneStack406 = (tubefilter->fConst1 * (0 - (0.022103400000000002 * toneStack401)));
	tubefilter->toneStack407 = (toneStack406 + (tubefilter->fConst2 * (toneStack400 + toneStack399)));
	tubefilter->toneStack408 = (toneStack406 - (tubefilter->fConst2 * (toneStack400 + toneStack404)));
	tubefilter->toneStack409 = ((toneStack31 == 10) / toneStack396);
	Real toneStack410 = (4.851e-08 * toneStack0);
	Real toneStack411 = (7.172000000000001e-07 + ((4.972000000000001e-05 * toneStack2) + (toneStack0 * (((4.8510000000000015e-06 * toneStack2) - 4.2449000000000006e-07) - toneStack410))));
	Real toneStack412 = ((2.6620000000000007e-10 * toneStack2) - (2.662e-12 * toneStack0));
	Real toneStack413 = (2.4200000000000003e-09 * toneStack2);
	Real toneStack414 = (2.4200000000000004e-11 + (toneStack413 + (toneStack0 * (toneStack412 - 2.1538000000000003e-11))));
	Real toneStack415 = (tubefilter->fConst1 * toneStack414);
	Real toneStack416 = (0.022050000000000004 * toneStack2);
	Real toneStack417 = (tubefilter->fConst1 * (0.0046705 + (toneStack339 + toneStack416)));
	tubefilter->toneStack418 = ((toneStack417 + (tubefilter->fConst2 * (toneStack415 - toneStack411))) - 1.0);
	Real toneStack419 = (tubefilter->fConst3 * toneStack414);
	tubefilter->toneStack420 = ((tubefilter->fConst2 * (toneStack411 + toneStack419)) - (3.0 + toneStack417));
	tubefilter->toneStack421 = ((toneStack417 + (tubefilter->fConst2 * (toneStack411 - toneStack419))) - 3.0);
	Real toneStack422 = (-(1.0 + (toneStack417 + (tubefilter->fConst2 * (toneStack411 + toneStack415)))));
	tubefilter->toneStack423 = (1.0 / toneStack422);
	Real toneStack424 = ((toneStack0 * (2.662e-12 + toneStack412)) + (toneStack17 * ((2.4200000000000004e-11 - (2.4200000000000004e-11 * toneStack0)) + toneStack413)));
	Real toneStack425 = (tubefilter->fConst3 * toneStack424);
	Real toneStack426 = (1.32e-08 + (((2.2000000000000004e-07 * toneStack17) + (toneStack0 * (5.951000000000001e-08 - toneStack410))) + (toneStack2 * (1.32e-06 + (4.8510000000000015e-06 * toneStack0)))));
	Real toneStack427 = (0.00022050000000000002 + (toneStack416 + (toneStack339 + (5e-05 * toneStack17))));
	Real toneStack428 = (tubefilter->fConst1 * toneStack427);
	tubefilter->toneStack429 = (toneStack428 + (tubefilter->fConst2 * (toneStack426 - toneStack425)));
	Real toneStack430 = (tubefilter->fConst1 * toneStack424);
	tubefilter->toneStack431 = (toneStack428 + (tubefilter->fConst2 * (toneStack430 - toneStack426)));
	Real toneStack432 = (tubefilter->fConst1 * (0 - toneStack427));
	tubefilter->toneStack433 = (toneStack432 + (tubefilter->fConst2 * (toneStack426 + toneStack425)));
	tubefilter->toneStack434 = (toneStack432 - (tubefilter->fConst2 * (toneStack426 + toneStack430)));
	tubefilter->toneStack435 = ((toneStack31 == 9) / toneStack422);
	Real toneStack436 = (1.38796875e-06 * toneStack0);
	Real toneStack437 = (3.5279375000000002e-06 + ((3.1989375e-05 * toneStack2) + (toneStack0 * (((1.38796875e-05 * toneStack2) - 1.6311937500000001e-06) - toneStack436))));
	Real toneStack438 = ((1.0561781250000004e-09 * toneStack2) - (1.0561781250000003e-10 * toneStack0));
	Real toneStack439 = (1.9328750000000005e-09 * toneStack2);
	Real toneStack440 = (1.9328750000000007e-10 + (toneStack439 + (toneStack0 * (toneStack438 - 8.766968750000004e-11))));
	Real toneStack441 = (tubefilter->fConst1 * toneStack440);
	Real toneStack442 = (0.001175 * toneStack0);
	Real toneStack443 = (0.011812500000000002 * toneStack2);
	Real toneStack444 = (tubefilter->fConst1 * (0.0065077500000000005 + (toneStack443 + toneStack442)));
	tubefilter->toneStack445 = ((toneStack444 + (tubefilter->fConst2 * (toneStack441 - toneStack437))) - 1);
	Real toneStack446 = (tubefilter->fConst3 * toneStack440);
	tubefilter->toneStack447 = ((tubefilter->fConst2 * (toneStack437 + toneStack446)) - (3 + toneStack444));
	tubefilter->toneStack448 = ((toneStack444 + (tubefilter->fConst2 * (toneStack437 - toneStack446))) - 3);
	Real toneStack449 = (0 - (1 + (toneStack444 + (tubefilter->fConst2 * (toneStack437 + toneStack441)))));
	tubefilter->toneStack450 = (1.0 / toneStack449);
	Real toneStack451 = ((toneStack0 * (1.0561781250000003e-10 + toneStack438)) + (toneStack17 * ((1.9328750000000007e-10 - (1.9328750000000007e-10 * toneStack0)) + toneStack439)));
	Real toneStack452 = (tubefilter->fConst3 * toneStack451);
	Real toneStack453 = (1.0633750000000002e-07 + (((3.2900000000000005e-07 * toneStack17) + (toneStack0 * (1.4614062500000001e-06 - toneStack436)))
		+ (toneStack2 * (1.0633750000000002e-06 + (1.38796875e-05 * toneStack0)))));
	Real toneStack454 = (toneStack21 + toneStack442);
	Real toneStack455 = (0.00118125 + (toneStack443 + toneStack454));
	Real toneStack456 = (tubefilter->fConst1 * toneStack455);
	tubefilter->toneStack457 = (toneStack456 + (tubefilter->fConst2 * (toneStack453 - toneStack452)));
	Real toneStack458 = (tubefilter->fConst1 * toneStack451);
	tubefilter->toneStack459 = (toneStack456 + (tubefilter->fConst2 * (toneStack458 - toneStack453)));
	Real toneStack460 = (tubefilter->fConst1 * -toneStack455);
	tubefilter->toneStack461 = (toneStack460 + (tubefilter->fConst2 * (toneStack453 + toneStack452)));
	tubefilter->toneStack462 = (toneStack460 - (tubefilter->fConst2 * (toneStack453 + toneStack458)));
	tubefilter->toneStack463 = ((toneStack31 == 8) / toneStack449);
	Real toneStack464 = (3.0937500000000006e-07 * toneStack0);
	Real toneStack465 = (1.2375000000000003e-05 * toneStack2);
	Real toneStack466 = (6.677000000000001e-07 + ((1.9448000000000004e-05 * toneStack2) + (toneStack0 * ((toneStack465 - 2.1175000000000003e-08) - toneStack464))));
	Real toneStack467 = ((1.7121500000000001e-09 * toneStack2) - (4.2803750000000003e-11 * toneStack0));
	Real toneStack468 = (1.9965000000000003e-09 * toneStack2);
	Real toneStack469 = (4.991250000000001e-11 + (toneStack468 + (toneStack0 * (toneStack467 - 7.108750000000004e-12))));
	Real toneStack470 = (tubefilter->fConst1 * toneStack469);
	Real toneStack471 = (0.022500000000000003 * toneStack2);
	Real toneStack472 = (toneStack8 + toneStack471);
	Real toneStack473 = (tubefilter->fConst1 * (0.0021395000000000003 + toneStack472));
	tubefilter->toneStack474 = ((toneStack473 + (tubefilter->fConst2 * (toneStack470 - toneStack466))) - 1.0);
	Real toneStack475 = (tubefilter->fConst3 * toneStack469);
	tubefilter->toneStack476 = ((tubefilter->fConst2 * (toneStack466 + toneStack475)) - (3.0 + toneStack473));
	tubefilter->toneStack477 = ((toneStack473 + (tubefilter->fConst2 * (toneStack466 - toneStack475))) - 3.0);
	Real toneStack478 = (-(1.0 + (toneStack473 + (tubefilter->fConst2 * (toneStack466 + toneStack470)))));
	tubefilter->toneStack479 = (1.0 / toneStack478);
	Real toneStack480 = ((toneStack0 * (4.2803750000000003e-11 + toneStack467)) + (toneStack17 * ((4.991250000000001e-11 - (4.991250000000001e-11 * toneStack0)) + toneStack468)));
	Real toneStack481 = (tubefilter->fConst3 * toneStack480);
	Real toneStack482 = (1.2375000000000003e-05 * toneStack0);
	Real toneStack483 = (toneStack0 * (3.781250000000001e-07 - toneStack464));
	Real toneStack484 = (8.690000000000002e-08 + (((1.815e-07 * toneStack17) + toneStack483) + (toneStack2 * (3.4760000000000007e-06 + toneStack482))));
	Real toneStack485 = (0.0005625000000000001 + (toneStack471 + (toneStack8 + (0.000125 * toneStack17))));
	Real toneStack486 = (tubefilter->fConst1 * toneStack485);
	tubefilter->toneStack487 = (toneStack486 + (tubefilter->fConst2 * (toneStack484 - toneStack481)));
	Real toneStack488 = (tubefilter->fConst1 * toneStack480);
	tubefilter->toneStack489 = (toneStack486 + (tubefilter->fConst2 * (toneStack488 - toneStack484)));
	Real toneStack490 = (tubefilter->fConst1 * -toneStack485);
	tubefilter->toneStack491 = (toneStack490 + (tubefilter->fConst2 * (toneStack484 + toneStack481)));
	tubefilter->toneStack492 = (toneStack490 - (tubefilter->fConst2 * (toneStack484 + toneStack488)));
	tubefilter->toneStack493 = ((toneStack31 == 7) / toneStack478);
	Real toneStack494 = (3.0621250000000006e-07 * toneStack0);
	Real toneStack495 = (5.442360000000002e-07 + ((1.784904e-05 * toneStack2) + (toneStack0 * (((1.2248500000000003e-05 * toneStack2) - 5.596250000000005e-08) - toneStack494))));
	Real toneStack496 = ((9.245610000000004e-10 * toneStack2) - (2.3114025000000008e-11 * toneStack0));
	Real toneStack497 = (1.0781100000000005e-09 * toneStack2);
	Real toneStack498 = (2.695275000000001e-11 + (toneStack497 + (toneStack0 * (toneStack496 - 3.8387250000000005e-12))));
	Real toneStack499 = (tubefilter->fConst1 * toneStack498);
	Real toneStack500 = (0.02227 * toneStack2);
	Real toneStack501 = (tubefilter->fConst1 * (0.00207625 + (toneStack8 + toneStack500)));
	tubefilter->toneStack502 = ((toneStack501 + (tubefilter->fConst2 * (toneStack499 - toneStack495))) - 1.0);
	Real toneStack503 = (tubefilter->fConst3 * toneStack498);
	tubefilter->toneStack504 = ((tubefilter->fConst2 * (toneStack495 + toneStack503)) - (3 + toneStack501));
	tubefilter->toneStack505 = ((toneStack501 + (tubefilter->fConst2 * (toneStack495 - toneStack503))) - 3.0);
	Real toneStack506 = (0 - (1.0 + (toneStack501 + (tubefilter->fConst2 * (toneStack495 + toneStack499)))));
	tubefilter->toneStack507 = (1.0 / toneStack506);
	Real toneStack508 = ((toneStack0 * (2.3114025000000008e-11 + toneStack496)) + (toneStack17 * ((2.695275000000001e-11 - (2.695275000000001e-11 * toneStack0)) + toneStack497)));
	Real toneStack509 = (tubefilter->fConst3 * toneStack508);
	Real toneStack510 = (4.6926e-08 + (((9.801000000000002e-08 * toneStack17) + (toneStack0 * (3.433375000000001e-07 - toneStack494))) + (toneStack2 * (1.8770400000000002e-06 + (1.2248500000000003e-05 * toneStack0)))));
	Real toneStack511 = (0.0005567500000000001 + (toneStack500 + (toneStack8 + toneStack296)));
	Real toneStack512 = (tubefilter->fConst1 * toneStack511);
	tubefilter->toneStack513 = (toneStack512 + (tubefilter->fConst2 * (toneStack510 - toneStack509)));
	Real toneStack514 = (tubefilter->fConst1 * toneStack508);
	tubefilter->toneStack515 = (toneStack512 + (tubefilter->fConst2 * (toneStack514 - toneStack510)));
	Real toneStack516 = (tubefilter->fConst1 * -toneStack511);
	tubefilter->toneStack517 = (toneStack516 + (tubefilter->fConst2 * (toneStack510 + toneStack509)));
	tubefilter->toneStack518 = (toneStack516 - (tubefilter->fConst2 * (toneStack510 + toneStack514)));
	tubefilter->toneStack519 = ((toneStack31 == 6) / toneStack506);
	Real toneStack520 = (1.08515e-06 + ((3.108600000000001e-05 * toneStack2) + (toneStack0 * ((toneStack465 - 2.99475e-07) - toneStack464))));
	Real toneStack521 = ((1.8513000000000002e-09 * toneStack2) - (4.628250000000001e-11 * toneStack0));
	Real toneStack522 = (3.3880000000000003e-09 * toneStack2);
	Real toneStack523 = (8.470000000000002e-11 + (toneStack522 + (toneStack0 * (toneStack521 - 3.8417500000000006e-11))));
	Real toneStack524 = (tubefilter->fConst1 * toneStack523);
	Real toneStack525 = (tubefilter->fConst1 * (toneStack472 + 0.0031515000000000002));
	tubefilter->toneStack526 = ((toneStack525 + (tubefilter->fConst2 * (toneStack524 - toneStack520))) - 1.0);
	Real toneStack527 = (tubefilter->fConst3 * toneStack523);
	tubefilter->toneStack528 = ((tubefilter->fConst2 * (toneStack520 + toneStack527)) - (3 + toneStack525));
	tubefilter->toneStack529 = ((toneStack525 + (tubefilter->fConst2 * (toneStack520 - toneStack527))) - 3.0);
	Real toneStack530 = (-(1.0 + (toneStack525 + (tubefilter->fConst2 * (toneStack520 + toneStack524)))));
	tubefilter->toneStack531 = (1.0 / toneStack530);
	Real toneStack532 = ((toneStack0 * (4.628250000000001e-11 + toneStack521)) + (toneStack17 * ((8.470000000000002e-11 - (8.470000000000002e-11 * toneStack0)) + toneStack522)));
	Real toneStack533 = (tubefilter->fConst3 * toneStack532);
	Real toneStack534 = (9.955000000000001e-08 + ((toneStack483 + (3.08e-07 * toneStack17)) + (toneStack2 * (toneStack482 + 3.982e-06))));
	tubefilter->toneStack535 = (toneStack486 + (tubefilter->fConst2 * (toneStack534 - toneStack533)));
	Real toneStack536 = (tubefilter->fConst1 * toneStack532);
	tubefilter->toneStack537 = (toneStack486 + (tubefilter->fConst2 * (toneStack536 - toneStack534)));
	tubefilter->toneStack538 = (toneStack490 + (tubefilter->fConst2 * (toneStack534 + toneStack533)));
	tubefilter->toneStack539 = (toneStack490 - (tubefilter->fConst2 * (toneStack534 + toneStack536)));
	tubefilter->toneStack540 = ((toneStack31 == 5) / toneStack530);
	Real toneStack541 = (5.665800800000001e-07 + ((1.892924e-05 * toneStack2) + (toneStack0 * ((toneStack142 - 6.207784000000001e-08) - toneStack141))));
	Real toneStack542 = ((1.2661536800000005e-09 * toneStack2) - (2.7855380960000008e-11 * toneStack0));
	Real toneStack543 = (1.6515048000000004e-09 * toneStack2);
	Real toneStack544 = (3.6333105600000014e-11 + (toneStack543 + (toneStack0 * (toneStack542 - 8.477724640000006e-12))));
	Real toneStack545 = (tubefilter->fConst1 * toneStack544);
	Real toneStack546 = (tubefilter->fConst1 * (toneStack150 + 0.0020497400000000004));
	tubefilter->toneStack547 = ((toneStack546 + (tubefilter->fConst2 * (toneStack545 - toneStack541))) - 1.0);
	Real toneStack548 = (tubefilter->fConst3 * toneStack544);
	tubefilter->toneStack549 = ((tubefilter->fConst2 * (toneStack541 + toneStack548)) - (3.0 + toneStack546));
	tubefilter->toneStack550 = ((toneStack546 + (tubefilter->fConst2 * (toneStack541 - toneStack548))) - 3.0);
	Real toneStack551 = (-(1.0 + (toneStack546 + (tubefilter->fConst2 * (toneStack541 + toneStack545)))));
	tubefilter->toneStack552 = (1.0 / toneStack551);
	Real toneStack553 = ((toneStack0 * (2.7855380960000008e-11 + toneStack542)) + (toneStack17 * ((3.6333105600000014e-11 - (3.6333105600000014e-11 * toneStack0)) + toneStack543)));
	Real toneStack554 = (tubefilter->fConst3 * toneStack553);
	Real toneStack555 = (6.505928000000001e-08 + ((toneStack161 + (1.5013680000000003e-07 * toneStack17)) + (toneStack2 * (toneStack160 + 2.95724e-06))));
	tubefilter->toneStack556 = (toneStack164 + (tubefilter->fConst2 * (toneStack555 - toneStack554)));
	Real toneStack557 = (tubefilter->fConst1 * toneStack553);
	tubefilter->toneStack558 = (toneStack164 + (tubefilter->fConst2 * (toneStack557 - toneStack555)));
	tubefilter->toneStack559 = (toneStack168 + (tubefilter->fConst2 * (toneStack555 + toneStack554)));
	tubefilter->toneStack560 = (toneStack168 - (tubefilter->fConst2 * (toneStack555 + toneStack557)));
	tubefilter->toneStack561 = ((toneStack31 == 4) / toneStack551);
	Real toneStack562 = (1.0855872000000003e-07 * toneStack0);
	Real toneStack563 = (3.222390000000001e-06 + (toneStack62 + (toneStack0 * (((5.6541000000000015e-06 * toneStack2) - 2.1333412800000006e-06) - toneStack562))));
	Real toneStack564 = (4.935e-10 * toneStack2);
	Real toneStack565 = (toneStack564 - (9.4752e-12 * toneStack0));
	Real toneStack566 = (1.41e-10 + (toneStack65 + (toneStack0 * (toneStack565 - 1.315248e-10))));
	Real toneStack567 = (tubefilter->fConst1 * toneStack566);
	Real toneStack568 = (0.0002256 * toneStack0);
	Real toneStack569 = (tubefilter->fConst1 * (0.015243699999999999 + (toneStack9 + toneStack568)));
	tubefilter->toneStack570 = ((toneStack569 + (tubefilter->fConst2 * (toneStack567 - toneStack563))) - 1.0);
	Real toneStack571 = (tubefilter->fConst3 * toneStack566);
	tubefilter->toneStack572 = ((tubefilter->fConst2 * (toneStack563 + toneStack571)) - (3.0 + toneStack569));
	tubefilter->toneStack573 = ((toneStack569 + (tubefilter->fConst2 * (toneStack563 - toneStack571))) - 3.0);
	Real toneStack574 = (-(1.0 + (toneStack569 + (tubefilter->fConst2 * (toneStack563 + toneStack567)))));
	tubefilter->toneStack575 = (1.0 / toneStack574);
	Real toneStack576 = (1.41e-10 - (1.41e-10 * toneStack0));
	Real toneStack577 = ((toneStack0 * (9.4752e-12 + toneStack565)) + (toneStack17 * (toneStack65 + toneStack576)));
	Real toneStack578 = (tubefilter->fConst3 * toneStack577);
	Real toneStack579 = (4.764000000000001e-08 + ((toneStack78 + (toneStack0 * (1.2265872000000003e-07 - toneStack562)))
		+ (toneStack2 * (2.48125e-06 + (5.6541000000000015e-06 * toneStack0)))));
	Real toneStack580 = (0.00048120000000000004 + (toneStack9 + (toneStack21 + toneStack568)));
	Real toneStack581 = (tubefilter->fConst1 * toneStack580);
	tubefilter->toneStack582 = (toneStack581 + (tubefilter->fConst2 * (toneStack579 - toneStack578)));
	Real toneStack583 = (tubefilter->fConst1 * toneStack577);
	tubefilter->toneStack584 = (toneStack581 + (tubefilter->fConst2 * (toneStack583 - toneStack579)));
	Real toneStack585 = (tubefilter->fConst1 * -toneStack580);
	tubefilter->toneStack586 = (toneStack585 + (tubefilter->fConst2 * (toneStack579 + toneStack578)));
	tubefilter->toneStack587 = (toneStack585 - (tubefilter->fConst2 * (toneStack579 + toneStack583)));
	tubefilter->toneStack588 = ((toneStack31 == 3) / toneStack574);
	Real toneStack589 = (4.7056400000000006e-07 * toneStack0);
	Real toneStack590 = (5.188640000000001e-06 + ((0.00011869100000000002 * toneStack2) + (toneStack0 * (((1.1764100000000001e-05 * toneStack2) - 4.215336e-06) - toneStack589))));
	Real toneStack591 = (toneStack564 - (1.974e-11 * toneStack0));
	Real toneStack592 = (3.525e-09 * toneStack2);
	Real toneStack593 = (1.41e-10 + (toneStack592 + (toneStack0 * (toneStack591 - 1.2126e-10))));
	Real toneStack594 = (tubefilter->fConst1 * toneStack593);
	Real toneStack595 = (0.02503 * toneStack2);
	Real toneStack596 = (tubefilter->fConst1 * (0.0157312 + (toneStack68 + toneStack595)));
	tubefilter->toneStack597 = ((toneStack596 + (tubefilter->fConst2 * (toneStack594 - toneStack590))) - 1.0);
	Real toneStack598 = (tubefilter->fConst3 * toneStack593);
	tubefilter->toneStack599 = ((tubefilter->fConst2 * (toneStack590 + toneStack598)) - (3 + toneStack596));
	tubefilter->toneStack600 = ((toneStack596 + (tubefilter->fConst2 * (toneStack590 - toneStack598))) - 3.0);
	Real toneStack601 = (-(1.0 + (toneStack596 + (tubefilter->fConst2 * (toneStack590 + toneStack594)))));
	tubefilter->toneStack602 = (1.0 / toneStack601);
	Real toneStack603 = ((toneStack0 * (1.974e-11 + toneStack591)) + (toneStack17 * (toneStack576 + toneStack592)));
	Real toneStack604 = (tubefilter->fConst3 * toneStack603);
	Real toneStack605 = (4.764000000000001e-08 + (((4.410000000000001e-07 * toneStack17) + (toneStack0 * (4.846640000000001e-07 - toneStack589)))
		+ (toneStack2 * (1.1910000000000001e-06 + (1.1764100000000001e-05 * toneStack0)))));
	Real toneStack606 = (0.0010012 + (toneStack595 + (toneStack68 + (3e-05 * toneStack17))));
	Real toneStack607 = (tubefilter->fConst1 * toneStack606);
	tubefilter->toneStack608 = (toneStack607 + (tubefilter->fConst2 * (toneStack605 - toneStack604)));
	Real toneStack609 = tubefilter->fConst1 * toneStack603;
	tubefilter->toneStack610 = (toneStack607 + (tubefilter->fConst2 * (toneStack609 - toneStack605)));
	Real toneStack611 = tubefilter->fConst1 * -toneStack606;
	tubefilter->toneStack612 = (toneStack611 + (tubefilter->fConst2 * (toneStack605 + toneStack604)));
	tubefilter->toneStack613 = (toneStack611 - (tubefilter->fConst2 * (toneStack605 + toneStack609)));
	tubefilter->toneStack614 = ((toneStack31 == 2) / toneStack601);
	Real toneStack615 = (2.9448437500000003e-06 * toneStack0);
	Real toneStack616 = (1.2916875000000002e-05 + (toneStack62 + (toneStack0 * (((2.9448437500000007e-05 * toneStack2) - 8.731718750000001e-06) - toneStack615))));
	Real toneStack617 = ((2.5703125000000004e-09 * toneStack2) - (2.5703125000000003e-10 * toneStack0));
	Real toneStack618 = (7.343750000000001e-10 + (toneStack65 + (toneStack0 * (toneStack617 - 4.773437500000001e-10))));
	Real toneStack619 = (tubefilter->fConst1 * toneStack618);
	Real toneStack620 = (tubefilter->fConst1 * (0.01726875 + (toneStack9 + toneStack442)));
	tubefilter->toneStack621 = ((toneStack620 + (tubefilter->fConst2 * (toneStack619 - toneStack616))) - 1.0);
	Real toneStack622 = tubefilter->fConst3 * toneStack618;
	tubefilter->toneStack623 = ((tubefilter->fConst2 * (toneStack616 + toneStack622)) - (3 + toneStack620));
	tubefilter->toneStack624 = ((toneStack620 + (tubefilter->fConst2 * (toneStack616 - toneStack622))) - 3.0);
	Real toneStack625 = (-(1.0 + (toneStack620 + (tubefilter->fConst2 * (toneStack616 + toneStack619)))));
	tubefilter->toneStack626 = (1.0 / toneStack625);
	Real toneStack627 = ((toneStack0 * (2.5703125000000003e-10 + toneStack617)) + (toneStack17 * (toneStack65 + (7.343750000000001e-10 - (7.343750000000001e-10 * toneStack0)))));
	Real toneStack628 = (tubefilter->fConst3 * toneStack627);
	Real toneStack629 = (2.48125e-07 + ((toneStack78 + (toneStack0 * (3.0182812500000004e-06 - toneStack615))) + (toneStack2 * (2.48125e-06 + (2.9448437500000007e-05 * toneStack0)))));
	Real toneStack630 = (0.0025062500000000002 + (toneStack9 + toneStack454));
	Real toneStack631 = (tubefilter->fConst1 * toneStack630);
	tubefilter->toneStack632 = (toneStack631 + (tubefilter->fConst2 * (toneStack629 - toneStack628)));
	Real toneStack633 = tubefilter->fConst1 * toneStack627;
	tubefilter->toneStack634 = (toneStack631 + (tubefilter->fConst2 * (toneStack633 - toneStack629)));
	Real toneStack635 = (tubefilter->fConst1 * -toneStack630);
	tubefilter->toneStack636 = (toneStack635 + (tubefilter->fConst2 * (toneStack629 + toneStack628)));
	tubefilter->toneStack637 = (toneStack635 - (tubefilter->fConst2 * (toneStack629 + toneStack633)));
	tubefilter->toneStack638 = ((toneStack31 == 1) / toneStack625);
	Real toneStack639 = (2.5312500000000006e-07 * toneStack0);
	Real toneStack640 = (7.4525e-07 + ((2.4210000000000004e-05 * toneStack2) + (toneStack0 * (((1.0125e-05 * toneStack2) - 2.75625e-07) - toneStack639))));
	Real toneStack641 = ((7.650000000000002e-10 * toneStack2) - (1.9125000000000002e-11 * toneStack0));
	Real toneStack642 = (1.4000000000000001e-09 * toneStack2);
	Real toneStack643 = (3.500000000000001e-11 + (toneStack642 + (toneStack0 * (toneStack641 - 1.5875000000000007e-11))));
	Real toneStack644 = (tubefilter->fConst1 * toneStack643);
	Real toneStack645 = (0.02025 * toneStack2);
	Real toneStack646 = (tubefilter->fConst1 * (0.0028087500000000005 + (toneStack365 + toneStack645)));
	tubefilter->toneStack647 = ((toneStack646 + (tubefilter->fConst2 * (toneStack644 - toneStack640))) - 1.0);
	Real toneStack648 = (tubefilter->fConst3 * toneStack643);
	tubefilter->toneStack649 = ((tubefilter->fConst2 * (toneStack640 + toneStack648)) - (3 + toneStack646));
	tubefilter->toneStack650 = ((toneStack646 + (tubefilter->fConst2 * (toneStack640 - toneStack648))) - 3.0);
	Real toneStack651 = (-(1.0 + (toneStack646 + (tubefilter->fConst2 * (toneStack640 + toneStack644)))));
	tubefilter->toneStack652 = 1.0 / toneStack651;
	Real toneStack653 = ((toneStack0 * (1.9125000000000002e-11 + toneStack641)) + (toneStack17 * ((3.500000000000001e-11 - (3.500000000000001e-11 * toneStack0)) + toneStack642)));
	Real toneStack654 = (tubefilter->fConst3 * toneStack653);
	Real toneStack655 = (4.525e-08 + (((1.4e-07 * toneStack17) + (toneStack0 * (2.8437500000000003e-07 - toneStack639))) + (toneStack2 * (1.8100000000000002e-06 + (1.0125e-05 * toneStack0)))));
	Real toneStack656 = (0.00050625 + (toneStack645 + (toneStack21 + toneStack365)));
	Real toneStack657 = (tubefilter->fConst1 * toneStack656);
	tubefilter->toneStack658 = (toneStack657 + (tubefilter->fConst2 * (toneStack655 - toneStack654)));
	Real toneStack659 = (tubefilter->fConst1 * toneStack653);
	tubefilter->toneStack660 = (toneStack657 + (tubefilter->fConst2 * (toneStack659 - toneStack655)));
	Real toneStack661 = (tubefilter->fConst1 * (0 - toneStack656));
	tubefilter->toneStack662 = (toneStack661 + (tubefilter->fConst2 * (toneStack655 + toneStack654)));
	tubefilter->toneStack663 = (toneStack661 - (tubefilter->fConst2 * (toneStack655 + toneStack659)));
	tubefilter->toneStack664 = ((toneStack31 == 0) / toneStack651);
	tubefilter->v1 = tubefilter->v2 = 0;
	return 1;
}
void processTube(tubeFilter *tubefilter, Real* inputs, Real* outputs, unsigned frames)
{
	Real tubeout;
	unsigned i, j, iMinus1;
	for (i = 0; i < frames; ++i)
	{
		//Step 1: read input sample as voltage for the source
		Real ViE = inputs[i] * tubefilter->overdrived1Gain;
		tubeout = (Real)advanc(&tubefilter->ckt, ViE) * tubefilter->overdrived2Gain;
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
