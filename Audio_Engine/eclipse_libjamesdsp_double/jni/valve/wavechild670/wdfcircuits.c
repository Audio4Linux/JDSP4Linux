#include "wdfcircuits.h"

TriodeRemoteCutoff6386 TriodeRemoteCutoff6386Init()
{
    TriodeRemoteCutoff6386 TRC6386;
    TRC6386.VgkLast = 1.0;
    TRC6386.numeratorLast = 0.0;
    TRC6386.fa = -0.1961135820501605;
    TRC6386.aa = 3.980508168e-08;
    TRC6386.ab = 2.3830020303;
    return TRC6386;
}
Real TriodeRemoteCutoff6386GetIa(Real Vgk, Real Vak)
{
    if (Vak < 0.0)
    {
        Vak = 0.0;
    }
    if (Vgk > 0.0)
    {
        Vgk = 0.0;
    }
    Real p1 = 3.981e-8;
    Real p2 = 2.383;
    Real p3 = 0.5;
    Real p4 = 0.1;
    Real p5 = 1.8;
    Real p6 = 0.5;
    Real p7 = -0.03922;
    Real p8 = 0.2;
    Real iakAlt = p1*pow(Vak, p2) / (pow((p3 - p4*Vgk), p5)*(p6 + exp(p7*Vak - p8*Vgk)));
    return iakAlt;
}
TriodeRemoteCutoff6386 TriodeRemoteCutoff6386InitClone(TriodeRemoteCutoff6386 other)
{
    TriodeRemoteCutoff6386 new6386 = TriodeRemoteCutoff6386Init();
    new6386.VgkLast = other.VgkLast;
    new6386.numeratorLast = other.numeratorLast;
    return new6386;
}
TriodeRemoteCutoff6386 clone(TriodeRemoteCutoff6386 TRC6386)
{
    return TriodeRemoteCutoff6386InitClone(TRC6386);
}
static inline Real TriodeRemoteCutoff6386getA(TriodeRemoteCutoff6386 *TRC6386, Real Vak)
{
    return TRC6386->aa*pow(Vak, TRC6386->ab);
}
static inline Real TriodeRemoteCutoff6386getF(TriodeRemoteCutoff6386 *TRC6386, Real Vak)
{
    return TRC6386->fa*Vak;
}

// WDFTubeInterface

void WDFTubeInterface3ArgInit(WDFTubeInterface *WDFTI, TriodeRemoteCutoff6386 model_, Real numParallelInstances)
{
    WDFTI->model = model_;
    if (numParallelInstances != -1.0)
        WDFTI->numParallelInstances = numParallelInstances; // May change-able
    else
        WDFTI->numParallelInstances = 3.0; // May change-able
    WDFTI->a = 0.0;
    WDFTI->Vgk = 0.0;
    WDFTI->Iak = 0.0;
    WDFTI->VakGuess = 100.0;
}

WDFTubeInterface WDFTubeInterface1ArgInit(WDFTubeInterface *other)
{
    WDFTubeInterface newWDFTubeInterface;
    TriodeRemoteCutoff6386 nModel = clone(other->model);
    newWDFTubeInterface.model = nModel;
    newWDFTubeInterface.numParallelInstances = other->numParallelInstances;
    newWDFTubeInterface.a = other->a;
    newWDFTubeInterface.Vgk = other->Vgk;
    newWDFTubeInterface.Iak = other->Iak;
    newWDFTubeInterface.VakGuess = other->VakGuess;
    return newWDFTubeInterface;
}
Real WDFTubeInterfaceEvaluateImplicitEquation(WDFTubeInterface *WDFTI, Real Vak)
{
    WDFTI->Iak = TriodeRemoteCutoff6386GetIa(WDFTI->Vgk, Vak) * WDFTI->numParallelInstances;
    return Vak + WDFTI->r0*WDFTI->Iak - WDFTI->a;
}
Real iterateNewtonRaphson(WDFTubeInterface *WDFTI, Real x, Real dx)
{
    /*
    x(n+1) = x(n) - Fn(x)/Fn'(x)

    Fn'(x) = dFn(x)/dx = (Fn(x + dx) - Fn(x))/(dx) #Finite difference approximation

    x(n+1) = x(n) - dx*Fn(x)/(Fn(x+dx) - Fn(x))
    */
    Real F = WDFTubeInterfaceEvaluateImplicitEquation(WDFTI, x);
    Real xNew = x - dx*F / (WDFTubeInterfaceEvaluateImplicitEquation(WDFTI, x + dx) - F);
    return xNew;
}
Real WDFTubeInterfaceGetB(WDFTubeInterface *WDFTI, Real a_, Real r0_, Real Vgate, Real Vk)
{
    /*
    Reference:
    "Wave Digital Simulation of a Vacuum-Tube Amplifier"
    By M. Karjalainen and J. Pakarinen, ICASSP 2006

    Vak + R0*f(Vgk, Vak) - a = 0 	#[Karjalainen and Pakarinen, eq 7]
    b = Vak - Ro*f(Vgk, Vak)		#[Karjalainen and Pakarinen, eq 8]
    */

    WDFTI->r0 = r0_;

    WDFTI->a = a_;

    WDFTI->Vgk = Vgate - Vk;

    Real Vak = WDFTI->VakGuess;
    uint iteration = 0;
    Real err = 1e6;
    WDFTI->Iak = 0.0;

//	printf("Vak=%3.14lf Vgk=%3.14lf a=%3.14lf\n", Vak, WDFTI->Vgk, WDFTI->a);
    while (fabs(err) / fabs(Vak) > 1e-9)   //1e-9 variable is precision adjustment
    {
        WDFTI->VakGuess = iterateNewtonRaphson(WDFTI, Vak, 1e-6);
        err = Vak - WDFTI->VakGuess;
        Vak = WDFTI->VakGuess;

//		printf("iteration:%d Vak=%3.14lf err=%3.14lf Iak=%3.14lf\n", iteration, Vak, err, WDFTI->Iak);
        if (iteration > 100)
        {
//			printf("Convergence failure!");
            break;
        }
        ++iteration;
    }
    Real b = Vak - WDFTI->r0*WDFTI->Iak;
    /*
    a = v + Ri
    b = v - Ri
    v = a + b
    */
//	printf("b=%3.14lf Vgk=%3.14lf Vak=%3.14lf Iak=%3.14lf\n", b, WDFTI->Vgk, Vak, WDFTI->Iak);
    return b;
}

void BidirectionalUnitDelayInterfaceInit(BidirectionalUnitDelayInterface * BUDI)
{
    BUDI->a = 0;
    BUDI->b = 0;
}
void TubeStageCircuitUpdateRValues(TubeStageCircuit *TSC, Real C_Ccathode, Real C_Cw, Real E_VcathodeBias, Real E_Vplate, Real L_Lm, Real L_Lp, Real L_Ls, Real NpOverNs, Real R_Rc, Real R_Routput, Real R_Rp, Real R_Rs, Real R_Rsidechain, Real R_VcathodeBias, Real R_Vplate, Real R_cathodeCapacitorConn, Real sampleRate)
{
    Real VcathodeBiasR = R_VcathodeBias;
    TSC->VcathodeBiasE = E_VcathodeBias;
    Real CcathodeR = 1.0 / (2.0*C_Ccathode*sampleRate);
    Real VplateR = R_Vplate;
    TSC->VplateE = E_Vplate;
    Real RoutputR = R_Routput;
    Real RsidechainR = R_Rsidechain;
    Real outputParallelConn_1R = RoutputR;
    Real outputParallelConn_2R = RsidechainR;
    Real outputParallelConn_3R = 1.0 / (1.0 / outputParallelConn_1R + 1.0 / outputParallelConn_2R);
    TSC->outputParallelConn_3Gamma1 = 1.0 / outputParallelConn_1R / (1.0 / outputParallelConn_1R + 1.0 / outputParallelConn_2R);
    Real LpR = 2.0*L_Lp*sampleRate;
    Real RpR = R_Rp;
    Real LmR = 2.0*L_Lm*sampleRate;
    Real RcR = R_Rc;
    Real LsR = 2.0*L_Ls*sampleRate;
    Real RsR = R_Rs;
    Real CwR = 1.0 / (2.0*C_Cw*sampleRate);
    TSC->transformern = 1.0 / NpOverNs;
    TSC->transformerOneOvern = NpOverNs;
    Real secondaryOutputParallelConn_1R = outputParallelConn_3R;
    Real secondaryOutputParallelConn_2R = CwR;
    Real secondaryOutputParallelConn_3R = 1.0 / (1.0 / secondaryOutputParallelConn_1R + 1.0 / secondaryOutputParallelConn_2R);
    TSC->secondaryOutputParallelConn_3Gamma1 = 1.0 / secondaryOutputParallelConn_1R / (1.0 / secondaryOutputParallelConn_1R + 1.0 / secondaryOutputParallelConn_2R);
    Real secondarySeriesConn2_3R = (RsR + LsR);
    TSC->secondarySeriesConn2_3Gamma1 = RsR / (RsR + LsR);
    TSC->secondarySeriesConn1_3Gamma1 = secondaryOutputParallelConn_3R / (secondaryOutputParallelConn_3R + secondarySeriesConn2_3R);
    Real transformerR = (secondaryOutputParallelConn_3R + secondarySeriesConn2_3R) / (TSC->transformern*TSC->transformern);
    Real primaryParallelConn1_1R = transformerR;
    Real primarySeriesConn2_3R = (LpR + RpR);
    TSC->primarySeriesConn2_3Gamma1 = LpR / (LpR + RpR);
    Real primaryParallelConn2_1R = LmR;
    Real primaryParallelConn2_2R = RcR;
    Real primaryParallelConn2_3R = 1.0 / (1.0 / primaryParallelConn2_1R + 1.0 / primaryParallelConn2_2R);
    TSC->primaryParallelConn2_3Gamma1 = 1.0 / primaryParallelConn2_1R / (1.0 / primaryParallelConn2_1R + 1.0 / primaryParallelConn2_2R);
    Real primaryParallelConn1_2R = primaryParallelConn2_3R;
    Real primaryParallelConn1_3R = 1.0 / (1.0 / primaryParallelConn1_1R + 1.0 / primaryParallelConn1_2R);
    TSC->primaryParallelConn1_3Gamma1 = 1.0 / primaryParallelConn1_1R / (1.0 / primaryParallelConn1_1R + 1.0 / primaryParallelConn1_2R);
    Real primaryInputSeriesConn_3R = (primarySeriesConn2_3R + primaryParallelConn1_3R);
    TSC->primaryInputSeriesConn_3Gamma1 = primarySeriesConn2_3R / (primarySeriesConn2_3R + primaryParallelConn1_3R);
    Real cathodeCapacitorConnR = R_cathodeCapacitorConn;
    Real cathodeCapSeriesConn_3R = (CcathodeR + cathodeCapacitorConnR);
    TSC->cathodeCapSeriesConn_3Gamma1 = CcathodeR / (CcathodeR + cathodeCapacitorConnR);
    Real cathodeParallelConn_1R = VcathodeBiasR;
    Real cathodeParallelConn_2R = cathodeCapSeriesConn_3R;
    Real cathodeParallelConn_3R = 1.0 / (1.0 / cathodeParallelConn_1R + 1.0 / cathodeParallelConn_2R);
    TSC->cathodeParallelConn_3Gamma1 = 1.0 / cathodeParallelConn_1R / (1.0 / cathodeParallelConn_1R + 1.0 / cathodeParallelConn_2R);
    Real tubeSeriesConn1_3R = (primaryInputSeriesConn_3R + VplateR);
    TSC->tubeSeriesConn1_3Gamma1 = primaryInputSeriesConn_3R / (primaryInputSeriesConn_3R + VplateR);
    Real tubeSeriesConn2_3R = (tubeSeriesConn1_3R + cathodeParallelConn_3R);
    TSC->tubeSeriesConn2_3Gamma1 = tubeSeriesConn1_3R / (tubeSeriesConn1_3R + cathodeParallelConn_3R);
    TSC->tubeR = tubeSeriesConn2_3R;
}
void TubeStageCircuitInit(TubeStageCircuit *TSC, Real C_Ccathode, Real C_Cw, Real E_VcathodeBias, Real E_Vplate, Real L_Lm, Real L_Lp, Real L_Ls, Real NpOverNs, Real R_Rc, Real R_Routput, Real R_Rp, Real R_Rs, Real R_Rsidechain, Real R_VcathodeBias, Real R_Vplate, Real R_cathodeCapacitorConn, Real sampleRate, WDFTubeInterface *tube_)
{
    TubeStageCircuitUpdateRValues(TSC, C_Ccathode, C_Cw, E_VcathodeBias, E_Vplate, L_Lm, L_Lp, L_Ls, NpOverNs, R_Rc, R_Routput, R_Rp, R_Rs, R_Rsidechain, R_VcathodeBias, R_Vplate, R_cathodeCapacitorConn, sampleRate);
    TSC->tube = WDFTubeInterface1ArgInit(tube_);
    TSC->Ccathodea = 0.0;
    TSC->Lpa = 0.0;
    TSC->Lma = 0.0;
    TSC->Lsa = 0.0;
    TSC->Cwa = 0.0;
    TSC->Vcathode = 0.0;
}

Real TubeStageCircuitAdvance(TubeStageCircuit *TSC, Real vgate, BidirectionalUnitDelayInterface *cathodeCapacitorConn)
{
    //Get Bs
    //tubeSeriesConn2_3GetB
    //tubeSeriesConn1_3GetB
    //primaryInputSeriesConn_3GetB
    //primarySeriesConn2_3GetB
    Real Lpb = -TSC->Lpa;
    //primarySeriesConn2_1SetA
    //RpGetB
    //primarySeriesConn2_2SetA
    Real primarySeriesConn2_3b3 = -(Lpb);
    //primaryInputSeriesConn_1SetA
    //primaryParallelConn1_3GetB
    //transformerGetB
    //secondarySeriesConn1_3GetB
    //secondaryOutputParallelConn_3GetB
    //outputParallelConn_3GetB
    //RoutputGetB
    //outputParallelConn_1SetA
    //RsidechainGetB
    //outputParallelConn_2SetA
    Real outputParallelConn_3b3 = -TSC->outputParallelConn_3Gamma1*(0.0);
    //secondaryOutputParallelConn_1SetA
    Real Cwb = TSC->Cwa;
    //secondaryOutputParallelConn_2SetA
    Real secondaryOutputParallelConn_3b3 = Cwb - TSC->secondaryOutputParallelConn_3Gamma1*(Cwb - outputParallelConn_3b3);
    //secondarySeriesConn1_1SetA
    //secondarySeriesConn2_3GetB
    //RsGetB
    //secondarySeriesConn2_1SetA
    Real Lsb = -TSC->Lsa;
    //secondarySeriesConn2_2SetA
    Real secondarySeriesConn2_3b3 = -(Lsb);
    //secondarySeriesConn1_2SetA
    Real secondarySeriesConn1_3b3 = -(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3);
    //primaryParallelConn1_1SetA
    //primaryParallelConn2_3GetB
    Real Lmb = -TSC->Lma;
    //primaryParallelConn2_1SetA
    //RcGetB
    //primaryParallelConn2_2SetA
    Real primaryParallelConn2_3b3 = -TSC->primaryParallelConn2_3Gamma1*(-Lmb);
    //primaryParallelConn1_2SetA
    Real primaryParallelConn1_3b3 = primaryParallelConn2_3b3 - TSC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TSC->transformerOneOvern);
    //primaryInputSeriesConn_2SetA
    Real primaryInputSeriesConn_3b3 = -(primarySeriesConn2_3b3 + primaryParallelConn1_3b3);
    //tubeSeriesConn1_1SetA
    //VplateGetB
    //tubeSeriesConn1_2SetA
    Real tubeSeriesConn1_3b3 = -(primaryInputSeriesConn_3b3 + TSC->VplateE);
    //tubeSeriesConn2_1SetA
    //cathodeParallelConn_3GetB
    //VcathodeBiasGetB
    //cathodeParallelConn_1SetA
    //cathodeCapSeriesConn_3GetB
    Real Ccathodeb = TSC->Ccathodea;
    //cathodeCapSeriesConn_1SetA
    //cathodeCapacitorConnGetB
    //cathodeCapSeriesConn_2SetA
    Real cathodeCapSeriesConn_3b3 = -(Ccathodeb + cathodeCapacitorConn->b);
    //cathodeParallelConn_2SetA
    Real cathodeParallelConn_3b3 = cathodeCapSeriesConn_3b3 - TSC->cathodeParallelConn_3Gamma1*(cathodeCapSeriesConn_3b3 - TSC->VcathodeBiasE);
    //tubeSeriesConn2_2SetA
    Real tubeSeriesConn2_3b3 = -(tubeSeriesConn1_3b3 + cathodeParallelConn_3b3);
    //Call tube model
    Real b = WDFTubeInterfaceGetB(&TSC->tube, tubeSeriesConn2_3b3, TSC->tubeR, vgate, TSC->Vcathode);
    //Set As
    //tubeSeriesConn2_3SetA
    Real tubeSeriesConn2_3b1 = tubeSeriesConn1_3b3 - TSC->tubeSeriesConn2_3Gamma1*(tubeSeriesConn1_3b3 + cathodeParallelConn_3b3 + b);
    //tubeSeriesConn1_3SetA
    Real tubeSeriesConn1_3b1 = primaryInputSeriesConn_3b3 - TSC->tubeSeriesConn1_3Gamma1*(primaryInputSeriesConn_3b3 + TSC->VplateE + tubeSeriesConn2_3b1);
    //primaryInputSeriesConn_3SetA
    Real primaryInputSeriesConn_3b1 = primarySeriesConn2_3b3 - TSC->primaryInputSeriesConn_3Gamma1*(primarySeriesConn2_3b3 + primaryParallelConn1_3b3 + tubeSeriesConn1_3b1);
    //primarySeriesConn2_3SetA
    Real primarySeriesConn2_3b1 = Lpb - TSC->primarySeriesConn2_3Gamma1*(Lpb + primaryInputSeriesConn_3b1);
    TSC->Lpa = primarySeriesConn2_3b1;
    //RpSetA
    Real primaryInputSeriesConn_3b2 = -(primarySeriesConn2_3b3 + tubeSeriesConn1_3b1 - TSC->primaryInputSeriesConn_3Gamma1*(primarySeriesConn2_3b3 + primaryParallelConn1_3b3 + tubeSeriesConn1_3b1));
    //primaryParallelConn1_3SetA
    Real primaryParallelConn1_3b1 = primaryInputSeriesConn_3b2 + primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TSC->transformerOneOvern - TSC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TSC->transformerOneOvern);
    //transformerSetA
    //secondarySeriesConn1_3SetA
    Real secondarySeriesConn1_3b1 = secondaryOutputParallelConn_3b3 - TSC->secondarySeriesConn1_3Gamma1*(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3 + primaryParallelConn1_3b1*TSC->transformern);
    //secondaryOutputParallelConn_3SetA
    //outputParallelConn_3SetA
    //RoutputSetA
    //RsidechainSetA
    Real secondaryOutputParallelConn_3b2 = secondarySeriesConn1_3b1 - TSC->secondaryOutputParallelConn_3Gamma1*(Cwb - outputParallelConn_3b3);
    TSC->Cwa = secondaryOutputParallelConn_3b2;
    Real secondarySeriesConn1_3b2 = -(secondaryOutputParallelConn_3b3 + primaryParallelConn1_3b1*TSC->transformern - TSC->secondarySeriesConn1_3Gamma1*(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3 + primaryParallelConn1_3b1*TSC->transformern));
    //secondarySeriesConn2_3SetA
    //RsSetA
    Real secondarySeriesConn2_3b2 = -(secondarySeriesConn1_3b2 - TSC->secondarySeriesConn2_3Gamma1*(Lsb + secondarySeriesConn1_3b2));
    TSC->Lsa = secondarySeriesConn2_3b2;
    Real primaryParallelConn1_3b2 = primaryInputSeriesConn_3b2 - TSC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TSC->transformerOneOvern);
    //primaryParallelConn2_3SetA
    Real primaryParallelConn2_3b1 = primaryParallelConn1_3b2 - Lmb - TSC->primaryParallelConn2_3Gamma1*(-Lmb);
    TSC->Lma = primaryParallelConn2_3b1;
    //RcSetA
    Real tubeSeriesConn2_3b2 = -(tubeSeriesConn1_3b3 + b - TSC->tubeSeriesConn2_3Gamma1*(tubeSeriesConn1_3b3 + cathodeParallelConn_3b3 + b));
    //cathodeParallelConn_3SetA
    Real cathodeParallelConn_3b2 = tubeSeriesConn2_3b2 - TSC->cathodeParallelConn_3Gamma1*(cathodeCapSeriesConn_3b3 - TSC->VcathodeBiasE);
    //cathodeCapSeriesConn_3SetA
    Real cathodeCapSeriesConn_3b1 = Ccathodeb - TSC->cathodeCapSeriesConn_3Gamma1*(Ccathodeb + cathodeCapacitorConn->b + cathodeParallelConn_3b2);
    TSC->Ccathodea = cathodeCapSeriesConn_3b1;
    Real cathodeCapSeriesConn_3b2 = -(Ccathodeb + cathodeParallelConn_3b2 - TSC->cathodeCapSeriesConn_3Gamma1*(Ccathodeb + cathodeCapacitorConn->b + cathodeParallelConn_3b2));
    //cathodeCapacitorConnSetA
    cathodeCapacitorConn->a = cathodeCapSeriesConn_3b2;
    TSC->Vcathode = -(TSC->Ccathodea + Ccathodeb);
    return -(TSC->Cwa + Cwb);
}

//
void TransformerCoupledInputCircuitUpdateRValues(TransformerCoupledInputCircuit *TCC, Real C_Cw, Real E_inputSource, Real L_Lm, Real L_Lp, Real L_Ls, Real NpOverNs, Real R_Rc, Real R_RinputTermination, Real R_Rload, Real R_Rp, Real R_Rs, Real R_inputSource, Real sampleRate)
{
    Real RloadR = R_Rload;
    Real inputSourceR = R_inputSource;
    TCC->inputSourceE = E_inputSource;
    Real RinputTerminationR = R_RinputTermination;
    Real LpR = 2.0*L_Lp*sampleRate;
    Real RpR = R_Rp;
    Real LmR = 2.0*L_Lm*sampleRate;
    Real RcR = R_Rc;
    Real LsR = 2.0*L_Ls*sampleRate;
    Real RsR = R_Rs;
    Real CwR = 1.0 / (2.0*C_Cw*sampleRate);
    TCC->transformern = 1.0 / NpOverNs;
    TCC->transformerOneOvern = NpOverNs;
    Real secondaryOutputParallelConn_1R = RloadR;
    Real secondaryOutputParallelConn_2R = CwR;
    Real secondaryOutputParallelConn_3R = 1.0 / (1.0 / secondaryOutputParallelConn_1R + 1.0 / secondaryOutputParallelConn_2R);
    TCC->secondaryOutputParallelConn_3Gamma1 = 1.0 / secondaryOutputParallelConn_1R / (1.0 / secondaryOutputParallelConn_1R + 1.0 / secondaryOutputParallelConn_2R);
    Real secondarySeriesConn2_3R = (RsR + LsR);
    TCC->secondarySeriesConn2_3Gamma1 = RsR / (RsR + LsR);
    TCC->secondarySeriesConn1_3Gamma1 = secondaryOutputParallelConn_3R / (secondaryOutputParallelConn_3R + secondarySeriesConn2_3R);
    Real transformerR = (secondaryOutputParallelConn_3R + secondarySeriesConn2_3R) / (TCC->transformern*TCC->transformern);
    Real primaryParallelConn1_1R = transformerR;
    Real primarySeriesConn2_3R = (LpR + RpR);
    TCC->primarySeriesConn2_3Gamma1 = LpR / (LpR + RpR);
    Real primaryParallelConn2_1R = LmR;
    Real primaryParallelConn2_2R = RcR;
    Real primaryParallelConn2_3R = 1.0 / (1.0 / primaryParallelConn2_1R + 1.0 / primaryParallelConn2_2R);
    TCC->primaryParallelConn2_3Gamma1 = 1.0 / primaryParallelConn2_1R / (1.0 / primaryParallelConn2_1R + 1.0 / primaryParallelConn2_2R);
    Real primaryParallelConn1_2R = primaryParallelConn2_3R;
    Real primaryParallelConn1_3R = 1.0 / (1.0 / primaryParallelConn1_1R + 1.0 / primaryParallelConn1_2R);
    TCC->primaryParallelConn1_3Gamma1 = 1.0 / primaryParallelConn1_1R / (1.0 / primaryParallelConn1_1R + 1.0 / primaryParallelConn1_2R);
    Real primaryInputSeriesConn_3R = (primarySeriesConn2_3R + primaryParallelConn1_3R);
    TCC->primaryInputSeriesConn_3Gamma1 = primarySeriesConn2_3R / (primarySeriesConn2_3R + primaryParallelConn1_3R);
    Real parallelConn_3R = (primaryInputSeriesConn_3R + RinputTerminationR);
    TCC->parallelConn_3Gamma1 = primaryInputSeriesConn_3R / (primaryInputSeriesConn_3R + RinputTerminationR);
    TCC->seriesConn_3Gamma1 = parallelConn_3R / (parallelConn_3R + inputSourceR);
}
void TransformerCoupledInputCircuitInit(TransformerCoupledInputCircuit *TCC, Real C_Cw, Real E_inputSource, Real L_Lm, Real L_Lp, Real L_Ls, Real NpOverNs, Real R_Rc, Real R_RinputTermination, Real R_Rload, Real R_Rp, Real R_Rs, Real R_inputSource, Real sampleRate)
{
    TransformerCoupledInputCircuitUpdateRValues(TCC, C_Cw, E_inputSource, L_Lm, L_Lp, L_Ls, NpOverNs, R_Rc, R_RinputTermination, R_Rload, R_Rp, R_Rs, R_inputSource, sampleRate);
    TCC->Lpa = 0.0;
    TCC->Lma = 0.0;
    TCC->Lsa = 0.0;
    TCC->Cwa = 0.0;
}

Real TransformerCoupledInputCircuitAdvance(TransformerCoupledInputCircuit *TCC, Real vin)
{
    TCC->inputSourceE = vin;
    //Get Bs
    //seriesConn_3GetB
    //parallelConn_3GetB
    //primaryInputSeriesConn_3GetB
    //primarySeriesConn2_3GetB
    Real Lpb = -TCC->Lpa;
    //primarySeriesConn2_1SetA
    //RpGetB
    //primarySeriesConn2_2SetA
    Real primarySeriesConn2_3b3 = -(Lpb);
    //primaryInputSeriesConn_1SetA
    //primaryParallelConn1_3GetB
    //transformerGetB
    //secondarySeriesConn1_3GetB
    //secondaryOutputParallelConn_3GetB
    //RloadGetB
    //secondaryOutputParallelConn_1SetA
    Real Cwb = TCC->Cwa;
    //secondaryOutputParallelConn_2SetA
    Real secondaryOutputParallelConn_3b3 = Cwb - TCC->secondaryOutputParallelConn_3Gamma1*(Cwb);
    //secondarySeriesConn1_1SetA
    //secondarySeriesConn2_3GetB
    //RsGetB
    //secondarySeriesConn2_1SetA
    Real Lsb = -TCC->Lsa;
    //secondarySeriesConn2_2SetA
    Real secondarySeriesConn2_3b3 = -(Lsb);
    //secondarySeriesConn1_2SetA
    Real secondarySeriesConn1_3b3 = -(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3);
    //primaryParallelConn1_1SetA
    //primaryParallelConn2_3GetB
    Real Lmb = -TCC->Lma;
    //primaryParallelConn2_1SetA
    //RcGetB
    //primaryParallelConn2_2SetA
    Real primaryParallelConn2_3b3 = -TCC->primaryParallelConn2_3Gamma1*(-Lmb);
    //primaryParallelConn1_2SetA
    Real primaryParallelConn1_3b3 = primaryParallelConn2_3b3 - TCC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TCC->transformerOneOvern);
    //primaryInputSeriesConn_2SetA
    Real primaryInputSeriesConn_3b3 = -(primarySeriesConn2_3b3 + primaryParallelConn1_3b3);
    //parallelConn_1SetA
    //RinputTerminationGetB
    //parallelConn_2SetA
    Real parallelConn_3b3 = -(primaryInputSeriesConn_3b3);
    //seriesConn_1SetA
    //inputSourceGetB
    //seriesConn_2SetA
    Real seriesConn_3b3 = -(parallelConn_3b3 + TCC->inputSourceE);
    Real b = -(seriesConn_3b3);
    //Set As
    //seriesConn_3SetA
    Real seriesConn_3b1 = parallelConn_3b3 - TCC->seriesConn_3Gamma1*(parallelConn_3b3 + TCC->inputSourceE + b);
    //parallelConn_3SetA
    Real parallelConn_3b1 = primaryInputSeriesConn_3b3 - TCC->parallelConn_3Gamma1*(primaryInputSeriesConn_3b3 + seriesConn_3b1);
    //primaryInputSeriesConn_3SetA
    Real primaryInputSeriesConn_3b1 = primarySeriesConn2_3b3 - TCC->primaryInputSeriesConn_3Gamma1*(primarySeriesConn2_3b3 + primaryParallelConn1_3b3 + parallelConn_3b1);
    //primarySeriesConn2_3SetA
    Real primarySeriesConn2_3b1 = Lpb - TCC->primarySeriesConn2_3Gamma1*(Lpb + primaryInputSeriesConn_3b1);
    TCC->Lpa = primarySeriesConn2_3b1;
    //RpSetA
    Real primaryInputSeriesConn_3b2 = -(primarySeriesConn2_3b3 + parallelConn_3b1 - TCC->primaryInputSeriesConn_3Gamma1*(primarySeriesConn2_3b3 + primaryParallelConn1_3b3 + parallelConn_3b1));
    //primaryParallelConn1_3SetA
    Real primaryParallelConn1_3b1 = primaryInputSeriesConn_3b2 + primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TCC->transformerOneOvern - TCC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TCC->transformerOneOvern);
    //transformerSetA
    //secondarySeriesConn1_3SetA
    Real secondarySeriesConn1_3b1 = secondaryOutputParallelConn_3b3 - TCC->secondarySeriesConn1_3Gamma1*(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3 + primaryParallelConn1_3b1*TCC->transformern);
    //secondaryOutputParallelConn_3SetA
    //RloadSetA
    Real secondaryOutputParallelConn_3b2 = secondarySeriesConn1_3b1 - TCC->secondaryOutputParallelConn_3Gamma1*(Cwb);
    TCC->Cwa = secondaryOutputParallelConn_3b2;
    Real secondarySeriesConn1_3b2 = -(secondaryOutputParallelConn_3b3 + primaryParallelConn1_3b1*TCC->transformern - TCC->secondarySeriesConn1_3Gamma1*(secondaryOutputParallelConn_3b3 + secondarySeriesConn2_3b3 + primaryParallelConn1_3b1*TCC->transformern));
    //secondarySeriesConn2_3SetA
    //RsSetA
    Real secondarySeriesConn2_3b2 = -(secondarySeriesConn1_3b2 - TCC->secondarySeriesConn2_3Gamma1*(Lsb + secondarySeriesConn1_3b2));
    TCC->Lsa = secondarySeriesConn2_3b2;
    Real primaryParallelConn1_3b2 = primaryInputSeriesConn_3b2 - TCC->primaryParallelConn1_3Gamma1*(primaryParallelConn2_3b3 - secondarySeriesConn1_3b3*TCC->transformerOneOvern);
    //primaryParallelConn2_3SetA
    Real primaryParallelConn2_3b1 = primaryParallelConn1_3b2 - Lmb - TCC->primaryParallelConn2_3Gamma1*(-Lmb);
    TCC->Lma = primaryParallelConn2_3b1;
    //RcSetA
    //RinputTerminationSetA
    return -(TCC->Cwa + Cwb);
}

//
void LevelTimeConstantCircuitUpdateRValues(LevelTimeConstantCircuit *LTCC, Real C_C1, Real C_C2, Real C_C3, Real R_R1, Real R_R2, Real R_R3, Real sampleRate)
{
    Real R1R = R_R1;
    Real C1R = 1.0 / (2.0*C_C1*sampleRate);
    Real R2R = R_R2;
    Real C2R = 1.0 / (2.0*C_C2*sampleRate);
    Real R3R = R_R3;
    Real C3R = 1.0 / (2.0*C_C3*sampleRate);
    Real serialConn2_3R = (R2R + C2R);
    LTCC->serialConn2_3Gamma1 = R2R / (R2R + C2R);
    Real serialConn3_3R = (R3R + C3R);
    LTCC->serialConn3_3Gamma1 = R3R / (R3R + C3R);
    Real parallelConn23_1R = serialConn2_3R;
    Real parallelConn23_2R = serialConn3_3R;
    Real parallelConn23_3R = 1.0 / (1.0 / parallelConn23_1R + 1.0 / parallelConn23_2R);
    LTCC->parallelConn23_3Gamma1 = 1.0 / parallelConn23_1R / (1.0 / parallelConn23_1R + 1.0 / parallelConn23_2R);
    Real parallelConn1_1R = R1R;
    Real parallelConn1_2R = C1R;
    Real parallelConn1_3R = 1.0 / (1.0 / parallelConn1_1R + 1.0 / parallelConn1_2R);
    LTCC->parallelConn1_3Gamma1 = 1.0 / parallelConn1_1R / (1.0 / parallelConn1_1R + 1.0 / parallelConn1_2R);
    Real parallelConnInput_1R = parallelConn1_3R;
    Real parallelConnInput_2R = parallelConn23_3R;
    Real parallelConnInput_3R = 1.0 / (1.0 / parallelConnInput_1R + 1.0 / parallelConnInput_2R);
    LTCC->parallelConnInput_3Gamma1 = 1.0 / parallelConnInput_1R / (1.0 / parallelConnInput_1R + 1.0 / parallelConnInput_2R);
    LTCC->Rsource = parallelConnInput_3R;
}
void LevelTimeConstantCircuitInit(LevelTimeConstantCircuit *LTCC, Real C_C1, Real C_C2, Real C_C3, Real R_R1, Real R_R2, Real R_R3, Real sampleRate)
{
    LevelTimeConstantCircuitUpdateRValues(LTCC, C_C1, C_C2, C_C3, R_R1, R_R2, R_R3, sampleRate);
    LTCC->C1a = 0.0;
    LTCC->C2a = 0.0;
    LTCC->C3a = 0.0;
}

Real LevelTimeConstantCircuitAdvance(LevelTimeConstantCircuit *LTCC, Real Iin)
{
    //parallelConnInput_3GetB
    //parallelConn1_3GetB
    //R1GetB
    //parallelConn1_1SetA
    Real C1b = LTCC->C1a;
    //parallelConn1_2SetA
    Real parallelConn1_3b3 = C1b - LTCC->parallelConn1_3Gamma1*(C1b);
    //parallelConnInput_1SetA
    //parallelConn23_3GetB
    //serialConn2_3GetB
    //R2GetB
    //serialConn2_1SetA
    Real C2b = LTCC->C2a;
    //serialConn2_2SetA
    Real serialConn2_3b3 = -(C2b);
    //parallelConn23_1SetA
    //serialConn3_3GetB
    //R3GetB
    //serialConn3_1SetA
    Real C3b = LTCC->C3a;
    //serialConn3_2SetA
    Real serialConn3_3b3 = -(C3b);
    //parallelConn23_2SetA
    Real parallelConn23_3b3 = serialConn3_3b3 - LTCC->parallelConn23_3Gamma1*(serialConn3_3b3 - serialConn2_3b3);
    //parallelConnInput_2SetA
    Real parallelConnInput_3b3 = parallelConn23_3b3 - LTCC->parallelConnInput_3Gamma1*(parallelConn23_3b3 - parallelConn1_3b3);
    //Current source law
    Real e = Iin * LTCC->Rsource;
    Real b = (parallelConnInput_3b3)-2.0*e;
    //parallelConnInput_3SetA
    Real parallelConnInput_3b1 = b + parallelConn23_3b3 - parallelConn1_3b3 - LTCC->parallelConnInput_3Gamma1*(parallelConn23_3b3 - parallelConn1_3b3);
    //parallelConn1_3SetA
    //R1SetA
    Real parallelConn1_3b2 = parallelConnInput_3b1 - LTCC->parallelConn1_3Gamma1*(C1b);
    LTCC->C1a = parallelConn1_3b2;
    Real parallelConnInput_3b2 = b - LTCC->parallelConnInput_3Gamma1*(parallelConn23_3b3 - parallelConn1_3b3);
    //parallelConn23_3SetA
    Real parallelConn23_3b1 = parallelConnInput_3b2 + serialConn3_3b3 - serialConn2_3b3 - LTCC->parallelConn23_3Gamma1*(serialConn3_3b3 - serialConn2_3b3);
    //serialConn2_3SetA
    //R2SetA
    Real serialConn2_3b2 = -(parallelConn23_3b1 - LTCC->serialConn2_3Gamma1*(C2b + parallelConn23_3b1));
    LTCC->C2a = serialConn2_3b2;
    Real parallelConn23_3b2 = parallelConnInput_3b2 - LTCC->parallelConn23_3Gamma1*(serialConn3_3b3 - serialConn2_3b3);
    //serialConn3_3SetA
    //R3SetA
    Real serialConn3_3b2 = -(parallelConn23_3b2 - LTCC->serialConn3_3Gamma1*(C3b + parallelConn23_3b2));
    LTCC->C3a = serialConn3_3b2;
    return -(LTCC->C1a + C1b);
}
