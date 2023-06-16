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

#include <utils/DesktopServices.h>

FirstLaunchWizard::FirstLaunchWizard(AutostartManager *autostart, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::FirstLaunchWizard),
    _autostart(autostart)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    QTimer::singleShot(500, this, [this] {
		ui->p1_icon->startAnimation();
    });
    ui->p3_icon->startAnimation();
    ui->p3b_icon->startAnimation();
    ui->p4_icon->startAnimation();

	ui->stackedWidget->setAnimation(QEasingCurve::Type::OutCirc);
    connect(ui->p1_next, &QPushButton::clicked, this, [&] {
        ui->stackedWidget->slideInIdx(1);
	});
    connect(ui->p3_next, &QPushButton::clicked, this, [&] {
        // Note: p3b (crash report) has been removed and is now a placeholder
        ui->stackedWidget->slideInIdx(3);
	});
    connect(ui->p3b_next, &QPushButton::clicked, this, [&] {
        ui->stackedWidget->slideInIdx(3);
    });
    connect(ui->p4_next, &QPushButton::clicked, this, [&] {
		emit wizardFinished();
	});
    connect(ui->p4_telegram, &QPushButton::clicked, this, [this] {
        DesktopServices::openUrl("https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw", this);
    });

    ui->p3_systray_disable->setChecked(!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_enable->setChecked(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_minOnBoot->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));

    ui->p3_systray_minOnBoot->setChecked(_autostart->isEnabled());

    connect(ui->p3_systray_disable, &QRadioButton::clicked, this, &FirstLaunchWizard::onSystrayRadioSelected);
    connect(ui->p3_systray_enable,  &QRadioButton::clicked, this, &FirstLaunchWizard::onSystrayRadioSelected);
    connect(ui->p3_systray_minOnBoot, &QCheckBox::stateChanged, this, &FirstLaunchWizard::onSystrayAutostartToggled);

    connect(&AppConfig::instance(), &AppConfig::updated, this, &FirstLaunchWizard::onAppConfigUpdated);
}

FirstLaunchWizard::~FirstLaunchWizard()
{
    disconnect(&AppConfig::instance(), &AppConfig::updated, this, &FirstLaunchWizard::onAppConfigUpdated);
    delete ui;
}

void FirstLaunchWizard::onAppConfigUpdated(const AppConfig::Key &key, const QVariant &value)
{
    switch(key)
    {
        case AppConfig::AutoStartEnabled:
            ui->p3_systray_minOnBoot->setChecked(value.toBool());
        default:
            break;
    }
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
    AppConfig::instance().set(AppConfig::TrayIconEnabled, ui->p3_systray_enable->isChecked());
    ui->p3_systray_minOnBoot->setEnabled(ui->p3_systray_enable->isChecked());
}

void FirstLaunchWizard::onSystrayAutostartToggled(bool isChecked)
{
    _autostart->setEnabled(isChecked);
}
