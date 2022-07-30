/************************************************************************************
*
* Wavechild670 v0.1
*
* wavechild670.h
*
* By Peter Raffensperger 11 March 2014
*
* Reference:
* Toward a Wave Digital Filter Model of the Fairchild 670 Limiter, Raffensperger, P. A., (2012).
* Proc. of the 15th International Conference on Digital Audio Effects (DAFx-12),
* York, UK, September 17-21, 2012.
*
* Note:
* Fairchild (R) a registered trademark of Avid Technology, Inc., which is in no way associated or
* affiliated with the author.
*
* License:
* Wavechild670 is licensed under the GNU GPL v2 license. If you use this
* software in an academic context, we would appreciate it if you referenced the original
* paper.
*
************************************************************************************/



#ifndef WAVECHILD670_H
#define WAVECHILD670_H
#include "../Misc.h"
#include "amplifiers.h"

#define LEVELTC_CIRCUIT_DEFAULT_C_C1 2e-6
#define LEVELTC_CIRCUIT_DEFAULT_C_C2 8e-6
#define LEVELTC_CIRCUIT_DEFAULT_C_C3 20e-6
#define LEVELTC_CIRCUIT_DEFAULT_R_R1 220e3
#define LEVELTC_CIRCUIT_DEFAULT_R_R2 1e9
#define LEVELTC_CIRCUIT_DEFAULT_R_R3 1e9

typedef struct strWavechild670Parameters
{
    Real inputLevelA;
    Real ACThresholdA;
    uint timeConstantSelectA;
    Real DCThresholdA;

    Real inputLevelB;
    Real ACThresholdB;
    uint timeConstantSelectB;
    Real DCThresholdB;

    int sidechainLink;
    int isMidSide;
    int useFeedbackTopology;

    Real outputGain;
} Wavechild670Parameters;

typedef struct strWavechild670
{
    Real sampleRate;
    Real outputGain;

    Real VlevelCapA;
    Real VlevelCapB;

    Real inputLevelA;
    Real inputLevelB;

    int useFeedbackTopology;
    int isMidSide;
    int sidechainLink;
    SidechainAmplifier sidechainAmplifierA;
    SidechainAmplifier sidechainAmplifierB;
    LevelTimeConstantCircuit levelTimeConstantCircuitA;
    LevelTimeConstantCircuit levelTimeConstantCircuitB;
    VariableMuAmplifier signalAmplifierA;
    VariableMuAmplifier signalAmplifierB;
} Wavechild670;
Wavechild670Parameters Wavechild670ParametersInit(Real inputLevelA_, Real ACThresholdA_, uint timeConstantSelectA_, Real DCThresholdA_,
        Real inputLevelB_, Real ACThresholdB_, uint timeConstantSelectB_, Real DCThresholdB_,
        int sidechainLink_, int isMidSide_, int useFeedbackTopology_, Real outputGain_);
Wavechild670* Wavechild670Init(Real sampleRate_, Wavechild670Parameters *parameters);
void Wavechild670WarmUp(Wavechild670 *w670, Real warmUpTimeInSeconds);
void Wavechild670Process(Wavechild670 *w670, Real *VinputLeftArr, Real *VinputRightArr, Real *VoutLeftArr, Real *VoutRightArr, ulong numSamples);
void Wavechild670ProcessFloat(Wavechild670 *w670, float *VinputLeftArr, float *VinputRightArr, float *VoutLeftArr, float *VoutRightArr, ulong numSamples);

#endif
