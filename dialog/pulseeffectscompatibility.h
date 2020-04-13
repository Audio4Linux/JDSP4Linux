#ifndef PULSEEFFECTSCOMP_H
#define PULSEEFFECTSCOMP_H

#include <QWidget>
#include "config/appconfigwrapper.h"

class MainWindow;

namespace Ui {
class PulseeffectsCompatibility;
}

class PulseeffectsCompatibility : public QWidget
{
    Q_OBJECT

public:
    explicit PulseeffectsCompatibility(AppConfigWrapper* _appconf, MainWindow*,QWidget *parent = nullptr);
    ~PulseeffectsCompatibility();

signals:
    void wizardFinished();

private:
    Ui::PulseeffectsCompatibility *ui;
    AppConfigWrapper* appconf;
    bool lockslot = false;

    void refreshDevices();
};

#endif // PULSEEFFECTSCOMP_H
