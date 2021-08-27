#include "SettingsFragment.h"
#include "ui_MainWindow.h"
#include "ui_SettingsFragment.h"

#include "config/AppConfig.h"
#include "interface/dialog/PaletteEditor.h"
#include "interface/QMenuEditor.h"
#include "interface/TrayIcon.h"
#include "MainWindow.h"
#include "utils/AutoStartManager.h"

#include <QAudioDeviceInfo>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QProcess>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

using namespace std;
static bool lockslot = false;

SettingsFragment::SettingsFragment(TrayIcon *trayIcon,
                                   QWidget  *parent) :
	QDialog(parent),
	ui(new Ui::SettingsFragment),
	_trayIcon(trayIcon)
{
	ui->setupUi(this);

	QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

	/*
	 * Prepare TreeView
	 */
	ui->selector->setCurrentItem(ui->selector->findItems("General", Qt::MatchFlag::MatchExactly).first());
	ui->stackedWidget->setCurrentIndex(0);
	connect(ui->selector, static_cast<void (QTreeWidget::*)(QTreeWidgetItem*, QTreeWidgetItem*)>(&QTreeWidget::currentItemChanged), this, [this](QTreeWidgetItem *cur, QTreeWidgetItem*)
	{
		int toplevel_index = ui->selector->indexOfTopLevelItem(cur);

		switch (toplevel_index)
		{
			case -1:

				if (cur->text(0) == "Context Menu")
				{
				    ui->stackedWidget->setCurrentIndex(5);
				}

				if (cur->text(0) == "Advanced")
				{
				    ui->stackedWidget->setCurrentIndex(7);
				}

				break;
			case 5:
				// -- SA/ROOT
				ui->stackedWidget->setCurrentIndex(6);
				break;
			default:
				ui->stackedWidget->setCurrentIndex(toplevel_index);
		}
	});
	ui->selector->expandItem(ui->selector->findItems("Spectrum Analyser", Qt::MatchFlag::MatchExactly).first());
	ui->selector->expandItem(ui->selector->findItems("Systray", Qt::MatchFlag::MatchExactly).first());

	/*
	 * Prepare all combooxes
	 */
	ui->paletteSelect->addItem("Default",    "default");
	ui->paletteSelect->addItem("Black",      "black");
	ui->paletteSelect->addItem("Blue",       "blue");
	ui->paletteSelect->addItem("Dark",       "dark");
	ui->paletteSelect->addItem("Dark Blue",  "darkblue");
	ui->paletteSelect->addItem("Dark Green", "darkgreen");
	ui->paletteSelect->addItem("Honeycomb",  "honeycomb");
	ui->paletteSelect->addItem("Gray",       "gray");
	ui->paletteSelect->addItem("Green",      "green");
	ui->paletteSelect->addItem("Stone",      "stone");
	ui->paletteSelect->addItem("Custom",     "custom");

	for ( const auto &i : QStyleFactory::keys())
	{
		ui->themeSelect->addItem(i);
	}

	/*
	 * Refresh all input fields
	 */
	refreshAll();

	/*
	 * Connect all signals for page General
	 */
	connect(ui->glavafix, &QPushButton::clicked, this, [this]
	{
		AppConfig::instance().setGFix(ui->glavafix->isChecked());
	});
	connect(ui->muteonrestart, &QCheckBox::clicked, this, [this]
	{
		AppConfig::instance().setMuteOnRestart(ui->muteonrestart->isChecked());
	});
	connect(ui->run_first_launch, &QPushButton::clicked, this, [this]
	{
		emit closeClicked();
		QTimer::singleShot(300, this, &SettingsFragment::launchSetupWizard);
	});
	auto autofx_mode = [this]
					   {
						   if (lockslot)
						   {
							   return;
						   }

						   int mode = 0;

						   if (ui->aa_instant->isChecked())
						   {
							   mode = 0;
						   }
						   else if (ui->aa_release->isChecked())
						   {
							   mode = 1;
						   }

						   AppConfig::instance().setAutoFxMode(mode);
					   };
	connect(ui->aa_instant,    &QRadioButton::clicked, this, autofx_mode);
	connect(ui->aa_release,    &QRadioButton::clicked, this, autofx_mode);
	connect(ui->glavafix_help, &QPushButton::clicked,  this, [this]
	{
		QMessageBox::information(this, tr("Help"),
		                         tr("This fix kills GLava (desktop visualizer) and restarts it after a new config has been applied.\nThis prevents GLava to switch to another audio sink, while JamesDSP is restarting."));
	});
	/*
	 * Connect all signals for Session
	 */
	auto systray_sel = [this]
					   {
						   if (lockslot)
						   {
							   return;
						   }

						   int mode = 0;

						   if (ui->systray_r_none->isChecked())
						   {
							   mode = 0;
						   }
						   else if (ui->systray_r_showtray->isChecked())
						   {
							   mode = 1;
						   }

						   AppConfig::instance().setTrayMode(mode);
						   ui->systray_icon_box->setEnabled(mode);
						   ui->menu_edit->setEnabled(mode);
					   };
	connect(ui->systray_r_none,     &QRadioButton::clicked, this, systray_sel);
	connect(ui->systray_r_showtray, &QRadioButton::clicked, this, systray_sel);
	auto autostart_update = [this, autostart_path]
							{
								if (ui->systray_minOnBoot->isChecked())
								{
									AutostartManager::saveDesktopFile(autostart_path,
									                                  AppConfig::instance().getExecutablePath(),
									                                  ui->systray_autostartJDSP->isChecked(),
									                                  ui->systray_delay->isChecked());
								}
								else
								{
									QFile(autostart_path).remove();
								}

								ui->systray_autostartJDSP->setEnabled(ui->systray_minOnBoot->isChecked());
								ui->systray_delay->setEnabled(ui->systray_minOnBoot->isChecked());
							};
	connect(ui->systray_minOnBoot,     &QPushButton::clicked, this, autostart_update);
	connect(ui->systray_autostartJDSP, &QPushButton::clicked, this, autostart_update);
	connect(ui->systray_delay,         &QPushButton::clicked, this, autostart_update);
	/*
	 * Connect all signals for Interface
	 */
	connect(ui->themeSelect,           static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
	        this, [this](const QString &)
	{
		if (lockslot)
		{
		    return;
		}

		AppConfig::instance().setTheme(ui->themeSelect->itemText(ui->themeSelect->currentIndex()).toUtf8().constData());
	});
	connect(ui->paletteSelect, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, [this]
	{
		if (lockslot)
		{
		    return;
		}

		AppConfig::instance().setColorpalette(ui->paletteSelect->itemData(ui->paletteSelect->currentIndex()).toString());
		ui->paletteConfig->setEnabled(AppConfig::instance().getColorpalette() == "custom");
	});
	connect(ui->paletteConfig, &QPushButton::clicked, this, [this]
	{
		auto c = new class PaletteEditor (&AppConfig::instance(), this);
		c->setFixedSize(c->geometry().width(), c->geometry().height());
		c->show();
	});
	connect(ui->eq_alwaysdrawhandles, &QCheckBox::clicked, [this]()
	{
		AppConfig::instance().setEqualizerPermanentHandles(ui->eq_alwaysdrawhandles->isChecked());
	});
	/*
	 * Connect all signals for Default Paths
	 */
	connect(ui->saveirspath, &QPushButton::clicked, this, [this]
	{
		AppConfig::instance().setIrsPath(ui->irspath->text());
	});
	connect(ui->saveddcpath, &QPushButton::clicked, this, [this]
	{
		AppConfig::instance().setDDCPath(ui->ddcpath->text());
	});
	connect(ui->saveliveprogpath, &QPushButton::clicked, this, [this]
	{
		AppConfig::instance().setLiveprogPath(ui->liveprog_path->text());
	});
	connect(ui->liveprog_autoextract, &QCheckBox::clicked, [this]()
	{
		AppConfig::instance().setLiveprogAutoExtract(ui->liveprog_autoextract->isChecked());
	});
	connect(ui->liveprog_extractNow, &QPushButton::clicked, this, [this]
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, tr("Question"), tr("Do you want to override existing EEL scripts (if any)?"),
		                              QMessageBox::Yes | QMessageBox::No);

		emit requestEelScriptExtract(reply == QMessageBox::Yes, true);
	});

	/*
	 * Connect all signals for Devices
	 */
	auto deviceUpdated = [this]()
						 {
							 if (lockslot)
							 {
								 return;
							 }

							 QString absolute =
								 QFileInfo(AppConfig::instance().getDspConfPath()).absoluteDir().absolutePath();
							 QString devices(pathAppend(absolute, "devices.conf"));

							 if (ui->dev_mode_auto->isChecked())
							 {
								 QFile(devices).remove();
							 }
							 else
							 {
								 if (ui->dev_select->currentData() == "---")
								 {
									 return;
								 }

								 ConfigContainer *devconf = new ConfigContainer();
								 devconf->setConfigMap(ConfigIO::readFile(devices));
								 devconf->setValue("location", ui->dev_select->currentData());
								 ConfigIO::writeFile(devices, devconf->getConfigMap());
							 }
						 };

	connect(ui->dev_mode_auto,   &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->dev_mode_manual, &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->dev_select,      static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, deviceUpdated);

	/*
	 * Connect all signals for SA/ROOT
	 */
	connect(ui->sa_enable,       &QGroupBox::clicked,                                                                this, [this]()
	{
		AppConfig::instance().setSpectrumEnable(ui->sa_enable->isChecked());
		ui->spectrum_advanced->setEnabled(ui->sa_enable->isChecked());
		emit reopenSettings();
	});
	connect(ui->sa_bands, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
		AppConfig::instance().setSpectrumBands(number);
	});
	connect(ui->sa_minfreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
		AppConfig::instance().setSpectrumMinFreq(number);
	});
	connect(ui->sa_maxfreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
		AppConfig::instance().setSpectrumMaxFreq(number);
	});
	connect(ui->sa_input, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, [this](const QString &str)
	{
		if (lockslot)
		{
		    return;
		}

		AppConfig::instance().setSpectrumInput(str);
	});
	connect(ui->sa_type, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, [this](const QString &)
	{
		if (lockslot)
		{
		    return;
		}

		AppConfig::instance().setSpectrumShape(ui->sa_type->currentIndex());
	});

	/*
	 * Connect all signals for SA/Advanced
	 */
	connect(ui->sa_refresh, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
		AppConfig::instance().setSpectrumRefresh(number);
	});
	connect(ui->sa_multi, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double number)
	{
		AppConfig::instance().setSpectrumMultiplier(number);
	});
	connect(ui->sa_grid, &QCheckBox::clicked, this, [this]()
	{
		AppConfig::instance().setSpectrumGrid(ui->sa_grid->isChecked());
	});

	/*
	 * Connect all signals for Global
	 */
	connect(ui->close,  &QPushButton::clicked, this, &SettingsFragment::closeClicked);
	connect(ui->github, &QPushButton::clicked, this, [] {
		QDesktopServices::openUrl(QUrl("https://github.com/Audio4Linux/JDSP4Linux"));
	});
	connect(ui->menu_edit, &QMenuEditor::targetChanged, [this, trayIcon]
	{
		auto menu = ui->menu_edit->exportMenu();
		trayIcon->updateTrayMenu(menu);
	});
	connect(ui->menu_edit, &QMenuEditor::resetPressed, [this, trayIcon]
	{
		QMessageBox::StandardButton reply = QMessageBox::question(this, "Warning", "Do you really want to restore the default layout?",
		                                                          QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::Yes)
		{
		    ui->menu_edit->setTargetMenu(trayIcon->buildDefaultActions());
		    auto menu = ui->menu_edit->exportMenu();
		    trayIcon->updateTrayMenu(menu);
		}
	});
	ui->menu_edit->setSourceMenu(trayIcon->buildAvailableActions());

	/*
	 * Check for systray availability
	 */
#ifndef QT_NO_SYSTEMTRAYICON
	ui->systray_unsupported->hide();
#else
	ui->session->setEnabled(false);
#endif

	if (!QSystemTrayIcon::isSystemTrayAvailable())
	{
		ui->systray_unsupported->show();
		ui->session->setEnabled(false);
	}
}

SettingsFragment::~SettingsFragment()
{
	delete ui;
}

void SettingsFragment::refreshDevices()
{
	lockslot = true;
	ui->dev_select->clear();
	QString             absolute     =
		QFileInfo(AppConfig::instance().getDspConfPath()).absoluteDir().absolutePath();
	QFile               devices(pathAppend(absolute, "devices.conf"));
	bool                devmode_auto = !devices.exists();
	ui->dev_mode_auto->setChecked(devmode_auto);
	ui->dev_mode_manual->setChecked(!devmode_auto);

	QProcess            process;
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("LC_ALL", "C");
	process.setProcessEnvironment(env);
	process.start("sh", QStringList() << "-c" << "pactl list sinks | grep \'Name: \' -A1");
	process.waitForFinished(500);

	ConfigContainer *devconf = new ConfigContainer();
	devconf->setConfigMap(ConfigIO::readFile(pathAppend(absolute, "devices.conf")));
	QString          out     = process.readAllStandardOutput();
	ui->dev_select->addItem("...", "---");

	for (auto item : out.split("Name:"))
	{
		item.prepend("Name:");
		QRegularExpression      re(R"((?<=(Name:)\s)(?<name>.+)[\s\S]+(?<=(Description:)\s)(?<desc>.+))");
		QRegularExpressionMatch match = re.match(item, 0, QRegularExpression::PartialPreferCompleteMatch);

		if (match.hasMatch())
		{
			ui->dev_select->addItem(QString("%1 (%2)").arg(match.captured("desc")).arg(match.captured("name")),
			                        match.captured("name"));
		}
	}

	QString dev_location = devconf->getVariant("location", true).toString();

	if (dev_location.isEmpty())
	{
		ui->dev_select->setCurrentIndex(0);
	}
	else
	{
		bool notFound = true;

		for (int i = 0; i < ui->dev_select->count(); i++)
		{
			if (ui->dev_select->itemData(i) ==
			    dev_location)
			{
				notFound = false;
				ui->dev_select->setCurrentIndex(i);
				break;
			}
		}

		if (notFound)
		{
			QString name = QString("Unknown (%1)").arg(dev_location);
			ui->dev_select->addItem(name, dev_location);
			ui->dev_select->setCurrentText(name);
		}
	}

	lockslot = false;
}

void SettingsFragment::refreshAll()
{
	lockslot = true;
	QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

	ui->menu_edit->setTargetMenu(_trayIcon->getTrayMenu());
	ui->menu_edit->setIconStyle(AppConfig::instance().getWhiteIcons());

	ui->irspath->setText(AppConfig::instance().getIrsPath());
	ui->ddcpath->setText(AppConfig::instance().getDDCPath());
	ui->liveprog_path->setText(AppConfig::instance().getLiveprogPath());
	ui->muteonrestart->setChecked(AppConfig::instance().getMuteOnRestart());
	ui->glavafix->setChecked(AppConfig::instance().getGFix());

	ui->liveprog_autoextract->setChecked(AppConfig::instance().getLiveprogAutoExtract());

	updateInputSinks();

	QString qvT(AppConfig::instance().getTheme());
	int     indexT = ui->themeSelect->findText(qvT);

	if ( indexT != -1 )
	{
		ui->themeSelect->setCurrentIndex(indexT);
	}
	else
	{
		int index_fallback = ui->themeSelect->findText("Fusion");

		if ( index_fallback != -1 )
		{
			ui->themeSelect->setCurrentIndex(index_fallback);
		}
	}

	QVariant qvS2(AppConfig::instance().getColorpalette());
	int      index2 = ui->paletteSelect->findData(qvS2);

	if ( index2 != -1 )
	{
		ui->paletteSelect->setCurrentIndex(index2);
	}

	ui->paletteConfig->setEnabled(AppConfig::instance().getColorpalette() == "custom");

	ui->aa_instant->setChecked(!AppConfig::instance().getAutoFxMode()); // same here..
	ui->aa_release->setChecked(AppConfig::instance().getAutoFxMode());

	ui->systray_r_none->setChecked(!AppConfig::instance().getTrayMode());
	ui->systray_r_showtray->setChecked(AppConfig::instance().getTrayMode());
	ui->systray_icon_box->setEnabled(AppConfig::instance().getTrayMode());
	ui->menu_edit->setEnabled(AppConfig::instance().getTrayMode());

	bool autostart_enabled     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Exists);
	bool autostartjdsp_enabled = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::UsesJDSPAutostart);
	bool autostart_delayed     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Delayed);

	ui->systray_minOnBoot->setChecked(autostart_enabled);
	ui->systray_autostartJDSP->setEnabled(autostart_enabled);
	ui->systray_autostartJDSP->setChecked(autostartjdsp_enabled);
	ui->systray_delay->setEnabled(autostart_enabled);
	ui->systray_delay->setChecked(autostart_delayed);

	ui->eq_alwaysdrawhandles->setChecked(AppConfig::instance().getEqualizerPermanentHandles());

	refreshDevices();

	int   bands      = AppConfig::instance().getSpectrumBands();
	int   minfreq    = AppConfig::instance().getSpectrumMinFreq();
	int   maxfreq    = AppConfig::instance().getSpectrumMaxFreq();
	int   refresh    = AppConfig::instance().getSpectrumRefresh();
	float multiplier = AppConfig::instance().getSpectrumMultiplier();

	// Set default values if undefined
	if (bands == 0)
	{
		bands = 100;
	}

	if (maxfreq == 0)
	{
		maxfreq = 1000;
	}

	if (refresh == 0)
	{
		refresh = 10;
	}

	if (multiplier == 0)
	{
		multiplier = 0.15;
	}

	// Check boundaries
	if (bands < 5 )
	{
		bands = 5;
	}
	else if (bands > 300)
	{
		bands = 300;
	}

	if (minfreq < 0)
	{
		minfreq = 0;
	}
	else if (minfreq > 10000)
	{
		minfreq = 10000;
	}

	if (maxfreq < 100)
	{
		maxfreq = 100;
	}
	else if (maxfreq > 24000)
	{
		maxfreq = 24000;
	}

	if (refresh < 10)
	{
		refresh = 10;
	}
	else if (refresh > 500)
	{
		refresh = 500;
	}

	if (multiplier < 0.01)
	{
		multiplier = 0.01;
	}
	else if (multiplier > 1)
	{
		multiplier = 1;
	}

	if (maxfreq < minfreq)
	{
		maxfreq = minfreq + 100;
	}

	ui->sa_enable->setChecked(AppConfig::instance().getSpectrumEnable());
	ui->spectrum_advanced->setEnabled(AppConfig::instance().getSpectrumEnable());

	ui->sa_type->setCurrentIndex(AppConfig::instance().getSpectrumShape());
	ui->sa_bands->setValue(bands);
	ui->sa_minfreq->setValue(minfreq);
	ui->sa_maxfreq->setValue(maxfreq);
	ui->sa_grid->setChecked(AppConfig::instance().getSpectrumGrid());
	ui->sa_refresh->setValue(refresh);
	ui->sa_multi->setValue(multiplier);

	lockslot = false;
}

void SettingsFragment::updateButtonStyle(bool white)
{
	ui->menu_edit->setIconStyle(white);
}

void SettingsFragment::setVisible(bool visible)
{
	refreshDevices();
	QDialog::setVisible(visible);
}

void SettingsFragment::updateInputSinks()
{
	lockslot = true;
	ui->sa_input->clear();

	for ( const auto &dev: QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
	{
		ui->sa_input->addItem(dev.deviceName());
	}

	QString qvSA(AppConfig::instance().getSpectrumInput());
	int     indexSA = ui->sa_input->findText(qvSA);

	if ( indexSA != -1 )
	{
		ui->sa_input->setCurrentIndex(indexSA);
	}
	else
	{
		int index_fallback = ui->themeSelect->findText("default");

		if ( index_fallback != -1 )
		{
			ui->sa_input->setCurrentIndex(index_fallback);
		}
	}

	lockslot = false;
}