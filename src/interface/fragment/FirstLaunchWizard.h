#ifndef FIRSTLAUNCHWIZARD_H
#define FIRSTLAUNCHWIZARD_H

#include "config/AppConfig.h"
#include <QWidget>

class MainWindow;

namespace Ui
{
	class FirstLaunchWizard;
}

class FirstLaunchWizard :
	public QWidget
{
	Q_OBJECT

public:
	explicit FirstLaunchWizard(QWidget *parent = nullptr);
	~FirstLaunchWizard();

signals:
	void wizardFinished();

private:
	Ui::FirstLaunchWizard *ui;
	bool lockslot = false;

	void refreshDevices();

};

#endif // FIRSTLAUNCHWIZARD_H