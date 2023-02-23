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
    explicit FirstLaunchWizard(QWidget *parent = nullptr);
	~FirstLaunchWizard();

protected:
    void showEvent(QShowEvent* ev) override;

signals:
	void wizardFinished();

private slots:
    void onSystrayRadioSelected();
    void onSystrayAutostartToggled(bool isChecked);

private:
    Ui::FirstLaunchWizard *ui;

};

#endif // FIRSTLAUNCHWIZARD_H
