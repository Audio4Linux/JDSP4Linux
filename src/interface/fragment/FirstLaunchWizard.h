#ifndef FIRSTLAUNCHWIZARD_H
#define FIRSTLAUNCHWIZARD_H

#include "config/AppConfig.h"
#include <QWidget>

class IAudioService;
class AutostartManager;

namespace Ui
{
	class FirstLaunchWizard;
}

class FirstLaunchWizard :
	public QWidget
{
	Q_OBJECT

public:
    explicit FirstLaunchWizard(AutostartManager *autostart, QWidget *parent = nullptr);
	~FirstLaunchWizard();

protected:
    void showEvent(QShowEvent* ev) override;

signals:
	void wizardFinished();

private slots:
    void onSystrayRadioSelected();
    void onSystrayAutostartToggled(bool isChecked);
    void onAppConfigUpdated(const AppConfig::Key &key, const QVariant &value);

private:
    Ui::FirstLaunchWizard *ui;
    AutostartManager *_autostart;
};

#endif // FIRSTLAUNCHWIZARD_H
