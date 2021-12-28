#include <IAudioService.h>

#include "FirstLaunchWizard.h"
#include "ui_FirstLaunchWizard.h"

#include "interface/TrayIcon.h"
#include "MainWindow.h"
#include "utils/AutoStartManager.h"
#include "utils/Common.h"

#include <QDesktopServices>
#include <QDir>
#include <QEasingCurve>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QUrl>

FirstLaunchWizard::FirstLaunchWizard(IAudioService *audioService, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::FirstLaunchWizard),
    audioService(audioService)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

	QTimer::singleShot(500, [&] {
		ui->p1_icon->startAnimation();
    });
	ui->p3_icon->startAnimation();
	ui->p4_icon->startAnimation();

	ui->stackedWidget->setAnimation(QEasingCurve::Type::OutCirc);
	connect(ui->p1_next, &QPushButton::clicked, [&] {
        ui->stackedWidget->slideInIdx(1);
	});
	connect(ui->p3_next, &QPushButton::clicked, [&] {
        ui->stackedWidget->slideInIdx(2);
	});
	connect(ui->p4_next,     &QPushButton::clicked, [&] {
		emit wizardFinished();
	});
	connect(ui->p4_telegram, &QPushButton::clicked, [&] {
		QDesktopServices::openUrl(QUrl("https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw"));
	});

    ui->p3_systray_disable->setChecked(!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_enable->setChecked(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_minOnBoot->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));

    ui->p3_systray_minOnBoot->setChecked(AutostartManager::inspectDesktopFile(AutostartManager::getAutostartPath("jdsp-gui.desktop"),
                                                                              AutostartManager::Exists));

    connect(ui->p3_systray_disable, &QRadioButton::clicked, this, &FirstLaunchWizard::onSystrayRadioSelected);
    connect(ui->p3_systray_enable,  &QRadioButton::clicked, this, &FirstLaunchWizard::onSystrayRadioSelected);
    connect(ui->p3_systray_minOnBoot, &QCheckBox::stateChanged, this, &FirstLaunchWizard::onSystrayAutostartToggled);
}

FirstLaunchWizard::~FirstLaunchWizard()
{
    delete ui;
}

void FirstLaunchWizard::showEvent(QShowEvent *ev)
{
    QWidget::showEvent(ev);
    auto maxSize = ui->p1_container->size() * 1.3;
    ui->p1_container->setMaximumSize(maxSize);
    ui->p3_container->setMaximumSize(maxSize);
    ui->p4_container->setMaximumSize(maxSize);
}

void FirstLaunchWizard::onSystrayRadioSelected()
{
    if (lockslot)
    {
        return;
    }

    AppConfig::instance().set(AppConfig::TrayIconEnabled, ui->p3_systray_enable->isChecked());
    ui->p3_systray_minOnBoot->setEnabled(ui->p3_systray_enable->isChecked());
}

void FirstLaunchWizard::onSystrayAutostartToggled(bool isChecked)
{
    auto path = AutostartManager::getAutostartPath("jdsp-gui.desktop");
    if (isChecked)
    {
        AutostartManager::saveDesktopFile(path,
                                          AppConfig::instance().get<QString>(AppConfig::ExecutablePath),
                                          AutostartManager::inspectDesktopFile(path, AutostartManager::Delayed));
    }
    else
    {
        QFile(path).remove();
    }
}
