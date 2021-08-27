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


FirstLaunchWizard::FirstLaunchWizard(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FirstLaunchWizard)
{
	ui->setupUi(this);

	QTimer::singleShot(500, [&] {
		ui->p1_icon->startAnimation();
	});
	ui->p2_icon->startAnimation();
	ui->p3_icon->startAnimation();
	ui->p4_icon->startAnimation();

	ui->stackedWidget->setAnimation(QEasingCurve::Type::OutCirc);
	connect(ui->p1_next, &QPushButton::clicked, [&] {
		ui->stackedWidget->slideInIdx(1);
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

	auto deviceUpdated = [this]()
						 {
							 if (lockslot)
							 {
								 return;
							 }

							 QString devices(AppConfig::instance().getPath("devices.conf"));

							 if (ui->p2_dev_mode_auto->isChecked())
							 {
								 QFile(devices).remove();
							 }
							 else
							 {
								 if (ui->p2_dev_select->currentData() == "---")
								 {
									 return;
								 }

								 ConfigContainer *devconf = new ConfigContainer();
								 devconf->setConfigMap(ConfigIO::readFile(devices));
								 devconf->setValue("location", ui->p2_dev_select->currentData());
								 ConfigIO::writeFile(devices, devconf->getConfigMap());
							 }
						 };
	refreshDevices();


	ui->p3_systray_disable->setChecked(!AppConfig::instance().getTrayMode());
	ui->p3_systray_enable->setChecked(AppConfig::instance().getTrayMode());
	ui->p3_systray_icon_box->setEnabled(AppConfig::instance().getTrayMode());

	QString autostart_path        = AutostartManager::getAutostartPath("jdsp-gui.desktop");
	bool    autostart_enabled     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Exists);
	bool    autostartjdsp_enabled = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::UsesJDSPAutostart);

	ui->p3_systray_minOnBoot->setChecked(autostart_enabled);
	ui->p3_systray_autostartJDSP->setEnabled(autostart_enabled);
	ui->p3_systray_autostartJDSP->setChecked(autostartjdsp_enabled);

	auto systray_radio = [this] {
							 if (lockslot)
							 {
								 return;
							 }

							 int mode = 0;

							 if (ui->p3_systray_disable->isChecked())
							 {
								 mode = 0;
							 }
							 else if (ui->p3_systray_enable->isChecked())
							 {
								 mode = 1;
							 }

							 AppConfig::instance().setTrayMode(mode);

							 ui->p3_systray_icon_box->setEnabled(mode);
						 };

	connect(ui->p3_systray_disable, &QRadioButton::clicked, this, systray_radio);
	connect(ui->p3_systray_enable,  &QRadioButton::clicked, this, systray_radio);

	auto systray_autostart_radio = [this, autostart_path]
								   {
									   if (ui->p3_systray_minOnBoot->isChecked())
									   {
										   AutostartManager::saveDesktopFile(autostart_path, AppConfig::instance().getExecutablePath(),
										                                     ui->p3_systray_autostartJDSP->isChecked(),
										                                     AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Delayed));
									   }
									   else
									   {
										   QFile(autostart_path).remove();
									   }

									   ui->p3_systray_autostartJDSP->setEnabled(ui->p3_systray_minOnBoot->isChecked());
								   };

	connect(ui->p3_systray_minOnBoot,     &QPushButton::clicked,                                                              this, systray_autostart_radio);
	connect(ui->p3_systray_autostartJDSP, &QPushButton::clicked,                                                              this, systray_autostart_radio);

	connect(ui->p2_dev_mode_auto,         &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->p2_dev_mode_manual,       &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->p2_dev_select,            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, deviceUpdated);

}

FirstLaunchWizard::~FirstLaunchWizard()
{
	delete ui;
}

void FirstLaunchWizard::refreshDevices()
{
	lockslot = true;
	ui->p2_dev_select->clear();

	QFile               devices(AppConfig::instance().getPath("devices.conf"));
	bool                devmode_auto = !devices.exists();
	ui->p2_dev_mode_auto->setChecked(devmode_auto);
	ui->p2_dev_mode_manual->setChecked(!devmode_auto);

	QProcess            process;
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("LC_ALL", "C");
	process.setProcessEnvironment(env);
	process.start("sh", QStringList() << "-c" << "pactl list sinks | grep \'Name: \' -A1");
	process.waitForFinished(500);

	ConfigContainer *devconf = new ConfigContainer();
	devconf->setConfigMap(ConfigIO::readFile(AppConfig::instance().getPath("devices.conf")));
	QString          out     = process.readAllStandardOutput();
	ui->p2_dev_select->addItem("...", "---");

	for (auto item : out.split("Name:"))
	{
		item.prepend("Name:");
		QRegularExpression      re(R"((?<=(Name:)\s)(?<name>.+)[\s\S]+(?<=(Description:)\s)(?<desc>.+))");
		QRegularExpressionMatch match = re.match(item, 0, QRegularExpression::PartialPreferCompleteMatch);

		if (match.hasMatch())
		{
			ui->p2_dev_select->addItem(QString("%1 (%2)").arg(match.captured("desc")).arg(match.captured("name")),
			                           match.captured("name"));
		}
	}

	QString dev_location = devconf->getVariant("location", true).toString();

	if (dev_location.isEmpty())
	{
		ui->p2_dev_select->setCurrentIndex(0);
	}
	else
	{
		bool notFound = true;

		for (int i = 0; i < ui->p2_dev_select->count(); i++)
		{
			if (ui->p2_dev_select->itemData(i) ==
			    dev_location)
			{
				notFound = false;
				ui->p2_dev_select->setCurrentIndex(i);
				break;
			}
		}

		if (notFound)
		{
			QString name = QString("Unknown (%1)").arg(dev_location);
			ui->p2_dev_select->addItem(name, dev_location);
			ui->p2_dev_select->setCurrentText(name);
		}
	}

	lockslot = false;
}