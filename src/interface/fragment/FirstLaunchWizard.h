#ifndef FIRSTLAUNCHWIZARD_H
#define FIRSTLAUNCHWIZARD_H

#include "config/AppConfig.h"
#include <QWidget>

class IAudioService;

namespace Ui
{
	class FirstLaunchWizard;
}

class FirstLaunchWizard :
	public QWidget
{
	Q_OBJECT

public:
    explicit FirstLaunchWizard(IAudioService *audioService,QWidget *parent = nullptr);
	~FirstLaunchWizard();

signals:
	void wizardFinished();

protected:
    void resizeEvent(QResizeEvent* ev) override;

private:
	Ui::FirstLaunchWizard *ui;
	bool lockslot = false;
    IAudioService* audioService;

	void refreshDevices();

private slots:
    void onDeviceUpdated();

};

#endif // FIRSTLAUNCHWIZARD_H
