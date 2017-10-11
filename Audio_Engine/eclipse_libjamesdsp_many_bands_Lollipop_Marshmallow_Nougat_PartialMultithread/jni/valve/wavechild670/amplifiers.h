#ifndef AMPLIFIERS_H
#define AMPLIFIERS_H

#include "wdfcircuits.h"

#define CATHODE_CAPACITOR_CONN_R 1e-6

/*
Simulation of a variable-Mu tube amplifier using the 6386 remote-cutoff tube
Peter Raffensperger
2012

References:
"Wave Digital Simulation of a Vacuum-Tube Amplifier"
By M. Karjalainen and J. Pakarinen, ICASSP 2006

*/

// Sidechain Amp
typedef struct strSidechainAmplifier
{
    uint calls;
    uint currentOvers;

    Real ACThresholdProcessed;
    Real DCThresholdProcessed;

    TransformerCoupledInputCircuit inputCircuit;

    //Input stage
    Real RinSeriesValue;
    Real RinParallelValue;
    Real Lp;
    Real Rp;
    Real Rc;
    Real Lm;
    Real Rs;
    Real Ls;
    Real Cw;
    Real NpOverNs;
    Real RpotValue; // = 2*76e3 //include the 25k center tap resistor. The pot itis 100k. Two in series is the load seen by the sidechain input, while the input voltage to the sidechain amp is the voltage across just one of them.

    //DC Threshold stage, 12AX7 amplifier
    Real DCThresholdScaleFactor; // 12.2
    Real DCThresholdOffset; // 0.1
    Real VscScaleFactor; // -6.0

    //Drive stage, 12BH7 + 6973 amplifier stages
    Real overallVoltageGain; // 17 //17 seemed like the gain of the drive stage in my SPICE simulation, but this number was then empirically fiddled to better match the performance implied by Fairchild 670 manual (and to get more compression)
    Real finalOutputClipVoltage; // 100.0
    Real diodeDropX2; // 1.5 //Twice the diode voltage drop
    Real nominalOutputConductance; // 1.0/80.0 //ohms
    Real maxOutputCurrent; // 0.5 //amps

} SidechainAmplifier;

// Variable mu Amp
typedef struct strVariableMuAmplifier
{
    //Input circuit
    TransformerCoupledInputCircuit inputCircuit;
    BidirectionalUnitDelay cathodeCapacitorConn;

    //Amplifier
    WDFTubeInterface tubeModelInterface;
    TubeStageCircuit tubeAmpPull;
    TubeStageCircuit tubeAmpPush;

    //Input circuit
    Real RinputValue;
    Real RinputTerminationValue;
    Real inputTxLp;
    Real inputTxRp;
    Real inputTxRc;
    Real inputTxLm;
    Real inputTxRs;
    Real inputTxLs;
    Real inputTxCw;
    Real inputTxNpOverNs;
    Real RgateValue;
    Real VgateBiasConst;

    //Amplifier
    Real numTubeParallelInstances;

    Real RcathodeValue;
    Real CcathodeValue;  //Should be twice the number on the Fairchild 670 schematic because there's effectively two of these in series
    Real VcathodeBias;
    Real RoutputValue;
    Real RsidechainValue;  //should only be non-infinite in a feedback topology
    Real RplateValue;
    Real Vplate;

    Real outputTxLp;
    Real outputTxRp;
    Real outputTxRc;
    Real outputTxLm;
    Real outputTxRs;
    Real outputTxLs;
    Real outputTxCw;
    Real outputTxNpOverNs;
} VariableMuAmplifier;

void VariableMuAmplifierInit(VariableMuAmplifier *VMA, Real sampleRate);
Real VariableMuAmplifierAdvanceAndGetOutputVoltage(VariableMuAmplifier *VMA, Real inputVoltage, Real VlevelCap);

void SidechainAmplifierSetThresholds(SidechainAmplifier *scAmp, Real ACThresholdNew, Real DCThresholdNew);
void SidechainAmplifierInit(SidechainAmplifier *scAmp, Real sampleRate, Real ACThresholdNew, Real DCThresholdNew);
Real SidechainAmplifierAdvanceAndGetCurrent(SidechainAmplifier *scAmp, Real VinSidechain, Real VlevelCap);
#endif
