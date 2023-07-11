#include "LiquidMultiEqualizerWidget.h"

extern "C" {
#include <JdspImpResToolbox.h>
}

LiquidMultiEqualizerWidget::LiquidMultiEqualizerWidget(QWidget* parent) : BaseLiquidEqualizerWidget(15, 24, 20000, -12.0, 12.0, 128, 3, parent)
{
    cplxRe = new double[resolution()];
    cplxIm = new double[resolution()];
}

LiquidMultiEqualizerWidget::~LiquidMultiEqualizerWidget()
{
    delete [] cplxRe;
    delete [] cplxIm;
}

LiquidMultiEqualizerWidget::Mode LiquidMultiEqualizerWidget::getMode() const
{
    return mode;
}

void LiquidMultiEqualizerWidget::setMode(Mode newMode)
{
    mode = newMode;
    emit redrawRequired();
}

int LiquidMultiEqualizerWidget::getIirOrder() const
{
    return iirOrder;
}

void LiquidMultiEqualizerWidget::setIirOrder(int newIirOrder)
{
    iirOrder = newIirOrder;
    emit redrawRequired();
}

QVector<double> LiquidMultiEqualizerWidget::getFrequencyPoints()
{
    return QVector<double>({ 25.0f, 40.0f, 63.0f, 100.0f, 160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f });
}

void LiquidMultiEqualizerWidget::computeCurve(const double *freqs, double *gains, int resolution, double *dispFreq, float *response)
{
    switch(mode) {
    case FIR:
        ComputeEqResponse(freqs, gains, 1, resolution, dispFreq, response);
        break;

    case IIR:
        ComputeIIREqualizerCplx(48000, iirOrder, freqs, gains, resolution, dispFreq, cplxRe, cplxIm);
        ComputeIIREqualizerResponse(resolution, cplxRe, cplxIm, response);
        break;
    }
}
