#include "amplifiers.h"

// Sidechain Amp
void SidechainAmplifierSetThresholds(SidechainAmplifier *scAmp, Real ACThresholdNew, Real DCThresholdNew)
{
    scAmp->DCThresholdProcessed = -scAmp->DCThresholdScaleFactor*(DCThresholdNew + scAmp->DCThresholdOffset);
    scAmp->ACThresholdProcessed = 0.5*ACThresholdNew*ACThresholdNew; //A nice curve that approximates the piecewise linear taper on the centre-potted tap curve on the Fairchild 670
}
void SidechainAmplifierInit(SidechainAmplifier *scAmp, Real sampleRate, Real ACThresholdNew, Real DCThresholdNew)
{
    //Input stage
    scAmp->RinSeriesValue = 600;
    scAmp->RinParallelValue = 1360;
    scAmp->Lp = 2e-3;
    scAmp->Rp = 10.0;
    scAmp->Rc = 50e3;
    scAmp->Lm = 5.7;
    scAmp->Rs = 50.0;
    scAmp->Ls = 1e-3;
    scAmp->Cw = 10e-12;
    scAmp->NpOverNs = 1.0 / 17.0;
    scAmp->RpotValue = 2 * 76e3; //includes the 25k center tap resistor. The pot itself is 100k. Two in series is the load seen by the sidechain input, while the input voltage to the sidechain amp is the voltage across just one of them.

    //DC Threshold stage, 12AX7 amplifier
    scAmp->DCThresholdScaleFactor = 12.2;
    scAmp->DCThresholdOffset = 0.1;
    scAmp->VscScaleFactor = -6.0;

    //Drive stage, 12BH7 + 6973 amplifier stages
    scAmp->overallVoltageGain = 8.4; //Used to be 17
    scAmp->finalOutputClipVoltage = 100.0;
    scAmp->diodeDropX2 = 0.6; //Twice the diode voltage drop, germanium diodes
    scAmp->nominalOutputConductance = 1.0 / 160.0; //ohms. Used to be 80
    scAmp->maxOutputCurrent = 0.5; //amps
    SidechainAmplifierSetThresholds(scAmp, ACThresholdNew, DCThresholdNew);

    scAmp->calls = 0;
    scAmp->currentOvers = 0;
    TransformerCoupledInputCircuitInit(&scAmp->inputCircuit, scAmp->Cw, 0.0, scAmp->Lm, scAmp->Lp, scAmp->Ls, scAmp->NpOverNs, scAmp->Rc, scAmp->RinParallelValue, scAmp->RpotValue, scAmp->Rp, scAmp->Rs, scAmp->RinSeriesValue, sampleRate);
}

inline Real SidechainAmplifierGetDCThresholdStageVsc(SidechainAmplifier *scAmp, Real VgPlus)
{
    Real xp = log1p(exp(VgPlus + scAmp->DCThresholdProcessed));
    Real xm = log1p(exp(-VgPlus + scAmp->DCThresholdProcessed));
    Real x = xp - xm;
    return scAmp->VscScaleFactor*x;
}

inline Real SidechainAmplifierDiodeModel(SidechainAmplifier *scAmp, Real V)
{
    const Real b = 10.0 / scAmp->diodeDropX2;
    const Real c = 10.0;
    if (V < 20.0)
    {
        return log1p(exp(b*V - c)) / b;
    }
    else
    {
        return V - scAmp->diodeDropX2;
    }
}
inline Real SidechainAmplifierCurrentSaturation(SidechainAmplifier *scAmp, Real i)
{
    //One side-saturation (does not saturate negatives)
    const Real b = 10.0 / scAmp->maxOutputCurrent;
    const Real c = 10.0;
    Real isat = log1p(exp(b*i - c)) / b;
    isat = fmin(isat, i);
    if (i > scAmp->maxOutputCurrent)
    {
        scAmp->currentOvers += 1;
    }
    return i - isat;
}
inline Real SidechainAmplifierGetDriveStageCurrent(SidechainAmplifier *scAmp, Real Vdiff)
{
    Real current = SidechainAmplifierDiodeModel(scAmp, Vdiff) * scAmp->nominalOutputConductance;
    current = SidechainAmplifierCurrentSaturation(scAmp, current);
    return current;
}
inline Real clip(Real x, Real minVal, Real maxVal)   //constrains x to [minVal, maxVal]
{
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}
Real SidechainAmplifierAdvanceAndGetCurrent(SidechainAmplifier *scAmp, Real VinSidechain, Real VlevelCap)
{
    scAmp->calls++;
    Real VgPlus = scAmp->ACThresholdProcessed*TransformerCoupledInputCircuitAdvance(&scAmp->inputCircuit, VinSidechain);
    Real Vsc = SidechainAmplifierGetDCThresholdStageVsc(scAmp, VgPlus);
    Real Vamp = Vsc * scAmp->overallVoltageGain;
    Vamp = clip(Vamp, -scAmp->finalOutputClipVoltage, scAmp->finalOutputClipVoltage); //Voltage saturation of the final output stage
    Real Vdiff = fabs(Vamp) - VlevelCap;
    Real Iout = SidechainAmplifierGetDriveStageCurrent(scAmp, Vdiff);
    return Iout;
}

// Variable mu Amp
void VariableMuAmplifierInit(VariableMuAmplifier *VMA, Real sampleRate)
{
    VMA->RinputValue = 600.0;
    VMA->RinputTerminationValue = 360.0;
    VMA->inputTxLp = 4e-3;
    VMA->inputTxRp = 10.0;
    VMA->inputTxRc = 10e3;
    VMA->inputTxLm = 35.7;
    VMA->inputTxRs = 50.0;
    VMA->inputTxLs = 1e-3;
    VMA->inputTxCw = 210e-12;
    VMA->inputTxNpOverNs = 2.0 / 9.0;
    //1:9 from the Drip Fairchild 670 transformer docs, decreased to meet the desired distortion specs, possibly with good reason given that the two primary sides are connected together!
    VMA->RgateValue = 100e3;
    VMA->VgateBiasConst = -7.2;

    //Amplifier
    VMA->numTubeParallelInstances = 2.0;
    VMA->RcathodeValue = 705;
    VMA->CcathodeValue = 8e-6; //Should be twice the number on the Fairchild 670 schematic because there's effectively two of these in series
    VMA->VcathodeBias = -3.1;
    VMA->RoutputValue = 600.0;
    VMA->RsidechainValue = 1000.0; //should only be non-infinite in a feedback topology
    VMA->RplateValue = 33;
    VMA->Vplate = 240.0;

    VMA->outputTxLp = 100e-6;
    VMA->outputTxRp = 5.0;
    VMA->outputTxRc = VMA->inputTxRc;
    VMA->outputTxLm = VMA->inputTxLm;
    VMA->outputTxRs = 1.0;
    VMA->outputTxLs = 400e-6;
    VMA->outputTxCw = 1e-12;
    VMA->outputTxNpOverNs = 1.0 / VMA->inputTxNpOverNs; //Somehow this gets inverted in the wdf code
    TransformerCoupledInputCircuitInit(&VMA->inputCircuit, VMA->inputTxCw, 0.0, VMA->inputTxLm, VMA->inputTxLp, VMA->inputTxLs, VMA->inputTxNpOverNs, VMA->inputTxRc, VMA->RinputTerminationValue, VMA->RgateValue, VMA->inputTxRp, VMA->inputTxRs, VMA->RinputValue, sampleRate);
    TriodeRemoteCutoff6386 trco6386 = TriodeRemoteCutoff6386Init();
    WDFTubeInterface3ArgInit(&VMA->tubeModelInterface, trco6386, VMA->numTubeParallelInstances);
    BidirectionalUnitDelayInterfaceInit(&VMA->cathodeCapacitorConn.interface0);
    BidirectionalUnitDelayInterfaceInit(&VMA->cathodeCapacitorConn.interface1);
    TubeStageCircuitInit(&VMA->tubeAmpPush, VMA->CcathodeValue, VMA->outputTxCw, VMA->VcathodeBias, VMA->Vplate, VMA->outputTxLm, VMA->outputTxLp, VMA->outputTxLs, VMA->outputTxNpOverNs, VMA->outputTxRc, VMA->RoutputValue, VMA->outputTxRp, VMA->outputTxRs, VMA->RsidechainValue, VMA->RcathodeValue, VMA->RplateValue, CATHODE_CAPACITOR_CONN_R, sampleRate, &VMA->tubeModelInterface);
    TubeStageCircuitInit(&VMA->tubeAmpPull, VMA->CcathodeValue, VMA->outputTxCw, VMA->VcathodeBias, VMA->Vplate, VMA->outputTxLm, VMA->outputTxLp, VMA->outputTxLs, VMA->outputTxNpOverNs, VMA->outputTxRc, VMA->RoutputValue, VMA->outputTxRp, VMA->outputTxRs, VMA->RsidechainValue, VMA->RcathodeValue, VMA->RplateValue, CATHODE_CAPACITOR_CONN_R, sampleRate, &VMA->tubeModelInterface);
}
Real VariableMuAmplifierAdvanceAndGetOutputVoltage(VariableMuAmplifier *VMA, Real inputVoltage, Real VlevelCap)
{
    Real Vgate = TransformerCoupledInputCircuitAdvance(&VMA->inputCircuit, inputVoltage);
    Real VoutPush = TubeStageCircuitAdvance(&VMA->tubeAmpPush, VMA->VgateBiasConst - VlevelCap + Vgate, &VMA->cathodeCapacitorConn.interface0);
    Real VoutPull = TubeStageCircuitAdvance(&VMA->tubeAmpPull, VMA->VgateBiasConst - VlevelCap - Vgate, &VMA->cathodeCapacitorConn.interface1);
    return VoutPush - VoutPull;
}
