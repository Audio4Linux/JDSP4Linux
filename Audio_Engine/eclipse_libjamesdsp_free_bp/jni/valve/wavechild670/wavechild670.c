/************************************************************************************
*
* Wavechild670 v0.1
*
* wavechild670.cpp
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


#include <stdlib.h>
#include "wavechild670.h"

Wavechild670Parameters Wavechild670ParametersInit(Real inputLevelA_, Real ACThresholdA_, uint timeConstantSelectA_, Real DCThresholdA_,
        Real inputLevelB_, Real ACThresholdB_, uint timeConstantSelectB_, Real DCThresholdB_,
        int sidechainLink_, int isMidSide_, int useFeedbackTopology_, Real outputGain_)
{
    Wavechild670Parameters parameters;
    parameters.inputLevelA = inputLevelA_;
    parameters.ACThresholdA = ACThresholdA_;
    parameters.timeConstantSelectA = timeConstantSelectA_;
    parameters.DCThresholdA = DCThresholdA_;

    parameters.inputLevelB = inputLevelB_;
    parameters.ACThresholdB = ACThresholdB_;
    parameters.timeConstantSelectB = timeConstantSelectB_;
    parameters.DCThresholdB = DCThresholdB_;

    parameters.sidechainLink = sidechainLink_;
    parameters.isMidSide = isMidSide_;
    parameters.useFeedbackTopology = useFeedbackTopology_;
    parameters.outputGain = outputGain_;
    return parameters;
}
void Wavechild670Select670TimeConstants(Wavechild670 *w670, uint tcA, uint tcB)
{
	const Real levelTimeConstantCircuitComponentValues[6][6] =
	{
		/* C1,    C2,   C3,   R1,   R2,    R3 */
		{ 2e-6, 8e-6, 20e-6, 51.9e3, 10e9, 10e9 },
		{ 2e-6, 8e-6, 20e-6, 149.9e3, 10e9, 10e9 },
		{ 4e-6, 8e-6, 20e-6, 220e3, 10e9, 10e9 },
		{ 8e-6, 8e-6, 20e-6, 220e3, 10e9, 10e9 },
		{ 4e-6, 8e-6, 20e-6, 220e3, 100e3, 10e9 },
		{ 2e-6, 8e-6, 20e-6, 220e3, 100e3, 100e3 }
	};
    tcA -= 1;
    LevelTimeConstantCircuitUpdateRValues(&w670->levelTimeConstantCircuitA, levelTimeConstantCircuitComponentValues[tcA][0], levelTimeConstantCircuitComponentValues[tcA][1], levelTimeConstantCircuitComponentValues[tcA][2], levelTimeConstantCircuitComponentValues[tcA][3], levelTimeConstantCircuitComponentValues[tcA][4], levelTimeConstantCircuitComponentValues[tcA][5], w670->sampleRate);
    tcB -= 1;
    LevelTimeConstantCircuitUpdateRValues(&w670->levelTimeConstantCircuitA, levelTimeConstantCircuitComponentValues[tcB][0], levelTimeConstantCircuitComponentValues[tcB][1], levelTimeConstantCircuitComponentValues[tcB][2], levelTimeConstantCircuitComponentValues[tcB][3], levelTimeConstantCircuitComponentValues[tcB][4], levelTimeConstantCircuitComponentValues[tcB][5], w670->sampleRate);
}
void Wavechild670SetParameters(Wavechild670 *w670, Wavechild670Parameters *parameters)
{
    w670->inputLevelA = parameters->inputLevelA;
    SidechainAmplifierSetThresholds(&w670->sidechainAmplifierA, parameters->ACThresholdA, parameters->DCThresholdA);

    w670->inputLevelB = parameters->inputLevelB;
    SidechainAmplifierSetThresholds(&w670->sidechainAmplifierB, parameters->ACThresholdB, parameters->DCThresholdB);

    Wavechild670Select670TimeConstants(w670, parameters->timeConstantSelectA, parameters->timeConstantSelectB);

    w670->sidechainLink = parameters->sidechainLink;
    w670->isMidSide = parameters->isMidSide;
    w670->useFeedbackTopology = parameters->useFeedbackTopology;
    w670->outputGain = parameters->outputGain;
}
// Wavechild670
Wavechild670* Wavechild670Init(Real sampleRate_, Wavechild670Parameters *parameters)
{
    Wavechild670 *w670 = (Wavechild670*)malloc(sizeof(Wavechild670));
    w670->sampleRate = sampleRate_;
    w670->useFeedbackTopology = parameters->useFeedbackTopology;
    w670->isMidSide = parameters->isMidSide;
    w670->sidechainLink = parameters->sidechainLink;
    SidechainAmplifierInit(&w670->sidechainAmplifierA, w670->sampleRate, parameters->ACThresholdA, parameters->DCThresholdA);
    SidechainAmplifierInit(&w670->sidechainAmplifierB, w670->sampleRate, parameters->ACThresholdB, parameters->DCThresholdB);
    LevelTimeConstantCircuitInit(&w670->levelTimeConstantCircuitA, LEVELTC_CIRCUIT_DEFAULT_C_C1, LEVELTC_CIRCUIT_DEFAULT_C_C2, LEVELTC_CIRCUIT_DEFAULT_C_C3, LEVELTC_CIRCUIT_DEFAULT_R_R1, LEVELTC_CIRCUIT_DEFAULT_R_R2, LEVELTC_CIRCUIT_DEFAULT_R_R3, w670->sampleRate);
    LevelTimeConstantCircuitInit(&w670->levelTimeConstantCircuitB, LEVELTC_CIRCUIT_DEFAULT_C_C1, LEVELTC_CIRCUIT_DEFAULT_C_C2, LEVELTC_CIRCUIT_DEFAULT_C_C3, LEVELTC_CIRCUIT_DEFAULT_R_R1, LEVELTC_CIRCUIT_DEFAULT_R_R2, LEVELTC_CIRCUIT_DEFAULT_R_R3, w670->sampleRate);
    w670->VlevelCapA = 0.0;
    w670->VlevelCapB = 0.0;
    VariableMuAmplifierInit(&w670->signalAmplifierA, w670->sampleRate);
    VariableMuAmplifierInit(&w670->signalAmplifierB, w670->sampleRate);
    w670->inputLevelA = parameters->inputLevelA;
    w670->inputLevelB = parameters->inputLevelB;
    Wavechild670SetParameters(w670, parameters);
    return w670;
}

void Wavechild670AdvanceSidechain(Wavechild670 *w670, Real VinSidechainA, Real VinSidechainB)
{
    Real sidechainCurrentA = SidechainAmplifierAdvanceAndGetCurrent(&w670->sidechainAmplifierA, VinSidechainA, w670->VlevelCapA);
    Real sidechainCurrentB = SidechainAmplifierAdvanceAndGetCurrent(&w670->sidechainAmplifierB, VinSidechainB, w670->VlevelCapB);
    if (w670->sidechainLink)
    {
        Real sidechainCurrentTotal = (sidechainCurrentA + sidechainCurrentB) / 2.0;// #Effectively compute the two circuits in parallel, crude but effective (I haven't prove this is exactly right)
        Real VlevelCapAx = LevelTimeConstantCircuitAdvance(&w670->levelTimeConstantCircuitA, sidechainCurrentTotal);
        Real VlevelCapBx = LevelTimeConstantCircuitAdvance(&w670->levelTimeConstantCircuitB, sidechainCurrentTotal); // #maintain the voltage in circuit B in case the user disengages the link
        w670->VlevelCapA = (VlevelCapAx + VlevelCapBx) / 2.0;
        w670->VlevelCapB = (VlevelCapAx + VlevelCapBx) / 2.0;
    }
    else
    {
        w670->VlevelCapA = LevelTimeConstantCircuitAdvance(&w670->levelTimeConstantCircuitA, sidechainCurrentA);
        w670->VlevelCapB = LevelTimeConstantCircuitAdvance(&w670->levelTimeConstantCircuitB, sidechainCurrentB);
    }
}
void Wavechild670WarmUp(Wavechild670 *w670, Real warmUpTimeInSeconds)   //warmUpTimeInSeconds = 0.5
{
    ulong numSamples = (ulong)(warmUpTimeInSeconds*w670->sampleRate);
    for (ulong i = 0; i < numSamples / 2; ++i)
    {
        Real VoutA = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierA, 0.0, w670->VlevelCapA);
        Real VoutB = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierB, 0.0, w670->VlevelCapB);
    }
    for (ulong i = 0; i < numSamples / 2; ++i)
    {
        Real VoutA = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierA, 0.0, w670->VlevelCapA);
        Real VoutB = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierB, 0.0, w670->VlevelCapB);
        Wavechild670AdvanceSidechain(w670, VoutA, VoutB); //Feedback topology with implicit unit delay between the sidechain input and the output,
    }
}

void Wavechild670Process(Wavechild670 *w670, Real *VinputLeftArr, Real *VinputRightArr, Real *VoutLeftArr, Real *VoutRightArr, ulong numSamples)
{
    for (ulong i = 0; i < numSamples; i++)
    {
        Real VinputA = VinputLeftArr[i];
        Real VinputB = VinputRightArr[i];
        VinputA *= w670->inputLevelA;
        VinputB *= w670->inputLevelB;

        if (!w670->useFeedbackTopology)   // => Feedforward
        {
            Wavechild670AdvanceSidechain(w670, VinputA, VinputB); //Feedforward topology
        }
        Real VoutA = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierA, VinputA, w670->VlevelCapA);
        Real VoutB = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierB, VinputB, w670->VlevelCapB);
        if (w670->useFeedbackTopology)
        {
            Wavechild670AdvanceSidechain(w670, VoutA, VoutB); //Feedback topology with implicit unit delay between the sidechain input and the output,
            //and probably an implicit unit delay between the sidechain capacitor voltage input and the capacitor voltage
            //(at least they're not the proper WDF coupling between the two)
        }

        Real VoutLeft;
        Real VoutRight;

        if (w670->isMidSide)
        {
            VoutLeft = (VoutA + VoutB) / sqrt(2.0);
            VoutRight = (VoutA - VoutB) / sqrt(2.0);
        }
        else
        {
            VoutLeft = VoutA;
            VoutRight = VoutB;
        }
        VoutLeftArr[i] = VoutLeft * w670->outputGain;
        VoutRightArr[i] = VoutRight * w670->outputGain;
    }
}
void Wavechild670ProcessFloat(Wavechild670 *w670, float *VinputLeftArr, float *VinputRightArr, float *VoutLeftArr, float *VoutRightArr, ulong numSamples)
{
	for (ulong i = 0; i < numSamples; i++)
	{
		Real VinputA = (Real)VinputLeftArr[i];
		Real VinputB = (Real)VinputRightArr[i];
		VinputA *= w670->inputLevelA;
		VinputB *= w670->inputLevelB;

		if (!w670->useFeedbackTopology)   // => Feedforward
		{
			Wavechild670AdvanceSidechain(w670, VinputA, VinputB); //Feedforward topology
		}
		Real VoutA = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierA, VinputA, w670->VlevelCapA);
		Real VoutB = VariableMuAmplifierAdvanceAndGetOutputVoltage(&w670->signalAmplifierB, VinputB, w670->VlevelCapB);
		if (w670->useFeedbackTopology)
		{
			Wavechild670AdvanceSidechain(w670, VoutA, VoutB); //Feedback topology with implicit unit delay between the sidechain input and the output,
															  //and probably an implicit unit delay between the sidechain capacitor voltage input and the capacitor voltage
															  //(at least they're not the proper WDF coupling between the two)
		}

		Real VoutLeft;
		Real VoutRight;

		if (w670->isMidSide)
		{
			VoutLeft = (VoutA + VoutB) / sqrt(2.0);
			VoutRight = (VoutA - VoutB) / sqrt(2.0);
		}
		else
		{
			VoutLeft = VoutA;
			VoutRight = VoutB;
		}
		VoutLeftArr[i] = (float)(VoutLeft * w670->outputGain);
		VoutRightArr[i] = (float)(VoutRight * w670->outputGain);
	}
}
