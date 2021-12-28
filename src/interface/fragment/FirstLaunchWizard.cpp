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
	ui->p2_icon->startAnimation();
	ui->p3_icon->startAnimation();
	ui->p4_icon->startAnimation();

	ui->stackedWidget->setAnimation(QEasingCurve::Type::OutCirc);
	connect(ui->p1_next, &QPushButton::clicked, [&] {

    // always skip
#if 1 //defined(USE_PULSEAUDIO)
        // Pulseaudio: skip device selection
        ui->stackedWidget->slideInIdx(2);
#else
        ui->stackedWidget->slideInIdx(1);
#endif
	});
	connect(ui->p2_next, &QPushButton::clicked, [&] {
		ui->stackedWidget->slideInIdx(2);
	});
	connect(ui->p3_next, &QPushButton::clicked, [&] {
		ui->stackedWidget->slideInIdx(3);
	});
	connect(ui->p4_next,     &QPushButton::clicked, [&] {
		emit wizardFinished();
	});
	connect(ui->p4_telegram, &QPushButton::clicked, [&] {
		QDesktopServices::openUrl(QUrl("https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw"));
	});

	refreshDevices();

    ui->p3_systray_disable->setChecked(!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_enable->setChecked(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->p3_systray_minOnBoot->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));

	QString autostart_path        = AutostartManager::getAutostartPath("jdsp-gui.desktop");
	bool    autostart_enabled     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Exists);

    ui->p3_systray_minOnBoot->setChecked(autostart_enabled);

	auto systray_radio = [this] {
							 if (lockslot)
							 {
								 return;
							 }

                             AppConfig::instance().set(AppConfig::TrayIconEnabled, ui->p3_systray_enable->isChecked());
                             ui->p3_systray_minOnBoot->setEnabled(ui->p3_systray_enable->isChecked());
						 };

	connect(ui->p3_systray_disable, &QRadioButton::clicked, this, systray_radio);
	connect(ui->p3_systray_enable,  &QRadioButton::clicked, this, systray_radio);

	auto systray_autostart_radio = [this, autostart_path]
								   {
									   if (ui->p3_systray_minOnBoot->isChecked())
									   {
                                           AutostartManager::saveDesktopFile(autostart_path, AppConfig::instance().get<QString>(AppConfig::ExecutablePath),
										                                     AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Delayed));
									   }
									   else
									   {
										   QFile(autostart_path).remove();
									   }
								   };

	connect(ui->p3_systray_minOnBoot,     &QPushButton::clicked,                                                              this, systray_autostart_radio);

    connect(ui->p2_dev_mode_auto,         &QRadioButton::clicked,                                                             this, &FirstLaunchWizard::onDeviceUpdated);
    connect(ui->p2_dev_mode_manual,       &QRadioButton::clicked,                                                             this, &FirstLaunchWizard::onDeviceUpdated);
    connect(ui->p2_dev_select,            qOverload<int>(&QComboBox::currentIndexChanged), this, &FirstLaunchWizard::onDeviceUpdated);

}

FirstLaunchWizard::~FirstLaunchWizard()
{
    delete ui;
}

void FirstLaunchWizard::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
    ui->stackedWidget->setMinimumHeight(ev->size().height() * 0.7);
}

void FirstLaunchWizard::refreshDevices()
{
    lockslot = true;
    ui->p2_dev_select->clear();

    ui->p2_dev_mode_auto->setChecked(AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));
    ui->p2_dev_mode_manual->setChecked(!AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));

    auto devices = audioService->sinkDevices();

    ui->p2_dev_select->addItem("...", 0);
    for (const auto& device : devices)
    {
        ui->p2_dev_select->addItem(QString("%1 (%2)")
                                .arg(QString::fromStdString(device.description))
                                .arg(QString::fromStdString(device.name)), QString::fromStdString(device.name));
    }

    auto current = AppConfig::instance().get<QString>(AppConfig::AudioOutputDevice);

    bool notFound = true;

    for (int i = 0; i < ui->p2_dev_select->count(); i++)
    {
        if (ui->p2_dev_select->itemData(i) == current)
        {
            notFound = false;
            ui->p2_dev_select->setCurrentIndex(i);
            break;
        }
    }

    if (notFound)
    {
        QString name = QString("Unknown (%1)").arg(current);
        ui->p2_dev_select->addItem(name, current);
        ui->p2_dev_select->setCurrentText(name);
    }
    lockslot = false;
}

void FirstLaunchWizard::onDeviceUpdated()
{
    if (lockslot)
    {
        return;
    }

    AppConfig::instance().set(AppConfig::AudioOutputUseDefault, ui->p2_dev_mode_auto->isChecked());

    if (!ui->p2_dev_mode_auto->isChecked())
    {
        if (ui->p2_dev_select->currentData() == "---")
        {
            return;
        }

        AppConfig::instance().set(AppConfig::AudioOutputDevice, ui->p2_dev_select->currentData());
    }
}
