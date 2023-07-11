#ifndef LIQUIDMULTIEQUALIZERWIDGET_H
#define LIQUIDMULTIEQUALIZERWIDGET_H

#include <LiquidEqualizerWidget.h>
#include <QObject>

class LiquidMultiEqualizerWidget : public BaseLiquidEqualizerWidget
{
    Q_OBJECT
public:
    enum Mode {
        FIR = 0,
        IIR = 1
    };

    LiquidMultiEqualizerWidget(QWidget* parent = nullptr);
    ~LiquidMultiEqualizerWidget();
    virtual void computeCurve(const double *freqs, double *gains, int resolution, double *dispFreq, float *response) override;
    virtual QVector<double> getFrequencyPoints() override;

    Mode getMode() const;
    void setMode(Mode newMode);

    int getIirOrder() const;
    void setIirOrder(int newIirOrder);

private:
    Mode mode = Mode::FIR;
    int iirOrder = 2;

    double *cplxRe = nullptr;
    double *cplxIm = nullptr;
};

#endif // LIQUIDMULTIEQUALIZERWIDGET_H
