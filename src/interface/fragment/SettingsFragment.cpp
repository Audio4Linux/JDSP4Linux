#include <IAudioService.h>

#include "SettingsFragment.h"
#include "ui_MainWindow.h"
#include "ui_SettingsFragment.h"

#include "config/AppConfig.h"
#include "interface/dialog/PaletteEditor.h"
#include "interface/QMenuEditor.h"
#include "interface/TrayIcon.h"
#include "MainWindow.h"
#include "utils/AutoStartManager.h"

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
                                   IAudioService *audioService,
                                   QWidget  *parent) :
	QDialog(parent),
	ui(new Ui::SettingsFragment),
    _trayIcon(trayIcon),
    _audioService(audioService)
{
	ui->setupUi(this);

#ifdef USE_PULSEAUDIO
    ui->devices->setEnabled(false);
    ui->devices_group->setTitle("Select sink/device to be processed (PipeWire only)");
    ui->blocklistBox->setVisible(false);
#endif

    QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

	/*
	 * Prepare TreeView
	 */

    ui->selector->setCurrentItem(ui->selector->topLevelItem(0));
	ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget->repaint();
	connect(ui->selector, static_cast<void (QTreeWidget::*)(QTreeWidgetItem*, QTreeWidgetItem*)>(&QTreeWidget::currentItemChanged), this, [this](QTreeWidgetItem *cur, QTreeWidgetItem*)
	{
		int toplevel_index = ui->selector->indexOfTopLevelItem(cur);

		switch (toplevel_index)
		{
			case -1:

                if (cur->text(0) == "Context menu")
				{
                    ui->stackedWidget->setCurrentIndex(4);
				}

				if (cur->text(0) == "Advanced")
				{
                    ui->stackedWidget->setCurrentIndex(6);
				}

				break;
            case 4:
				// -- SA/ROOT
                //ui->stackedWidget->setCurrentIndex(5);
				break;
			default:
                ui->stackedWidget->setCurrentIndex(toplevel_index);
                break;
		}

        // Workaround: Force redraw
        ui->stackedWidget->hide();
        ui->stackedWidget->show();
        ui->stackedWidget->repaint();
	});
    //ui->selector->expandItem(ui->selector->findItems("Spectrum analyser", Qt::MatchFlag::MatchExactly).first());
    ui->selector->expandItem(ui->selector->findItems("Tray icon", Qt::MatchFlag::MatchExactly).first());

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
	 * Connect all signals for Session
	 */
	auto systray_sel = [this]
					   {
						   if (lockslot)
						   {
							   return;
						   }

                           AppConfig::instance().set(AppConfig::TrayIconEnabled, ui->systray_r_showtray->isChecked());
                           ui->systray_icon_box->setEnabled(ui->systray_r_showtray->isChecked());
                           ui->menu_edit->setEnabled(ui->systray_r_showtray->isChecked());
					   };
	connect(ui->systray_r_none,     &QRadioButton::clicked, this, systray_sel);
	connect(ui->systray_r_showtray, &QRadioButton::clicked, this, systray_sel);
	auto autostart_update = [this, autostart_path]
							{
								if (ui->systray_minOnBoot->isChecked())
								{
									AutostartManager::saveDesktopFile(autostart_path,
                                                                      AppConfig::instance().get<QString>(AppConfig::ExecutablePath),
									                                  ui->systray_delay->isChecked());
								}
								else
								{
									QFile(autostart_path).remove();
								}

								ui->systray_delay->setEnabled(ui->systray_minOnBoot->isChecked());
							};
    connect(ui->systray_minOnBoot,     &QPushButton::clicked, this, autostart_update);
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

        AppConfig::instance().set(AppConfig::Theme, ui->themeSelect->itemText(ui->themeSelect->currentIndex()).toUtf8().constData());
	});
	connect(ui->paletteSelect, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, [this]
	{
		if (lockslot)
		{
		    return;
		}

        AppConfig::instance().set(AppConfig::ThemeColors, ui->paletteSelect->itemData(ui->paletteSelect->currentIndex()).toString());
        ui->paletteConfig->setEnabled(AppConfig::instance().get<QString>(AppConfig::ThemeColors) == "custom");
	});
	connect(ui->paletteConfig, &QPushButton::clicked, this, [this]
	{
		auto c = new class PaletteEditor (&AppConfig::instance(), this);
		c->setFixedSize(c->geometry().width(), c->geometry().height());
		c->show();
	});
	connect(ui->eq_alwaysdrawhandles, &QCheckBox::clicked, [this]()
	{
        AppConfig::instance().set(AppConfig::EqualizerShowHandles, ui->eq_alwaysdrawhandles->isChecked());
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
        AppConfig::instance().set(AppConfig::LiveprogAutoExtract, ui->liveprog_autoextract->isChecked());
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

         AppConfig::instance().set(AppConfig::AudioOutputUseDefault, ui->dev_mode_auto->isChecked());

         if (!ui->dev_mode_auto->isChecked())
         {
             if (ui->dev_select->currentData() == "---")
             {
                 return;
             }

             AppConfig::instance().set(AppConfig::AudioOutputDevice, ui->dev_select->currentData());
         }
     };

	connect(ui->dev_mode_auto,   &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->dev_mode_manual, &QRadioButton::clicked,                                                             this, deviceUpdated);
	connect(ui->dev_select,      static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this, deviceUpdated);

	/*
	 * Connect all signals for SA/ROOT
	 */
    /*connect(ui->sa_enable,       &QGroupBox::clicked,                                                                this, [this]()
	{
        AppConfig::instance().set(AppConfig::SpectrumEnabled, ui->sa_enable->isChecked());
		ui->spectrum_advanced->setEnabled(ui->sa_enable->isChecked());
		emit reopenSettings();
	});
	connect(ui->sa_bands, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
        AppConfig::instance().set(AppConfig::SpectrumBands, number);
	});
	connect(ui->sa_minfreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
        AppConfig::instance().set(AppConfig::SpectrumMinFreq, number);
	});
	connect(ui->sa_maxfreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
        AppConfig::instance().set(AppConfig::SpectrumMaxFreq, number);
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

        AppConfig::instance().set(AppConfig::SpectrumTheme, ui->sa_type->currentIndex());
    });*/

	/*
	 * Connect all signals for SA/Advanced
	 */
    /*connect(ui->sa_refresh, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int number)
	{
        AppConfig::instance().set(AppConfig::SpectrumRefresh, number);
	});
	connect(ui->sa_multi, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double number)
	{
        AppConfig::instance().set(AppConfig::SpectrumMultiplier, number);
	});
	connect(ui->sa_grid, &QCheckBox::clicked, this, [this]()
	{
        AppConfig::instance().set(AppConfig::SpectrumGrid, ui->sa_grid->isChecked());
    });*/

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

    connect(ui->run_first_launch, &QPushButton::clicked, this, [this]
    {
        emit closeClicked();
        QTimer::singleShot(300, this, &SettingsFragment::launchSetupWizard);
    });
    connect(ui->blocklistClear, &QPushButton::clicked, this, []
    {
        AppConfig::instance().set(AppConfig::AudioAppBlocklist, "");
    });

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

    ui->dev_mode_auto->setChecked(AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));
    ui->dev_mode_manual->setChecked(!AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));

    auto devices = _audioService->sinkDevices();

    ui->dev_select->addItem("...", 0);
    for (const auto& device : devices)
    {
        ui->dev_select->addItem(QString("%1 (%2)")
                                .arg(QString::fromStdString(device.description))
                                .arg(QString::fromStdString(device.name)), QString::fromStdString(device.name));
    }

    auto current = AppConfig::instance().get<QString>(AppConfig::AudioOutputDevice);

    bool notFound = true;

    for (int i = 0; i < ui->dev_select->count(); i++)
    {
        if (ui->dev_select->itemData(i) == current)
        {
            notFound = false;
            ui->dev_select->setCurrentIndex(i);
            break;
        }
    }

    if (notFound)
    {
        QString name = QString("Unknown (%1)").arg(current);
        ui->dev_select->addItem(name, current);
        ui->dev_select->setCurrentText(name);
    }
	lockslot = false;
}

void SettingsFragment::refreshAll()
{
	lockslot = true;
	QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

	ui->menu_edit->setTargetMenu(_trayIcon->getTrayMenu());
    ui->menu_edit->setIconStyle(AppConfig::instance().get<bool>(AppConfig::ThemeColorsCustomWhiteIcons));

	ui->irspath->setText(AppConfig::instance().getIrsPath());
	ui->ddcpath->setText(AppConfig::instance().getDDCPath());
	ui->liveprog_path->setText(AppConfig::instance().getLiveprogPath());

    ui->liveprog_autoextract->setChecked(AppConfig::instance().get<bool>(AppConfig::LiveprogAutoExtract));

    QString qvT(AppConfig::instance().get<QString>(AppConfig::Theme));
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

    QVariant qvS2(AppConfig::instance().get<QString>(AppConfig::ThemeColors));
	int      index2 = ui->paletteSelect->findData(qvS2);

	if ( index2 != -1 )
	{
		ui->paletteSelect->setCurrentIndex(index2);
	}

    ui->paletteConfig->setEnabled(AppConfig::instance().get<QString>(AppConfig::ThemeColors) == "custom");

    ui->systray_r_none->setChecked(!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->systray_r_showtray->setChecked(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->systray_icon_box->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->menu_edit->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));

	bool autostart_enabled     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Exists);
	bool autostart_delayed     = AutostartManager::inspectDesktopFile(autostart_path, AutostartManager::Delayed);

    ui->systray_minOnBoot->setChecked(autostart_enabled);
	ui->systray_delay->setEnabled(autostart_enabled);
	ui->systray_delay->setChecked(autostart_delayed);

    ui->eq_alwaysdrawhandles->setChecked(AppConfig::instance().get<bool>(AppConfig::EqualizerShowHandles));

	refreshDevices();

    /*int   bands      = AppConfig::instance().get<int>(AppConfig::SpectrumBands);
    int   minfreq    = AppConfig::instance().get<int>(AppConfig::SpectrumMinFreq);
    int   maxfreq    = AppConfig::instance().get<int>(AppConfig::SpectrumMaxFreq);
    int   refresh    = AppConfig::instance().get<int>(AppConfig::SpectrumRefresh);
    float multiplier = AppConfig::instance().get<float>(AppConfig::SpectrumMultiplier);

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

    ui->sa_enable->setChecked(AppConfig::instance().get<bool>(AppConfig::SpectrumEnabled));
    ui->spectrum_advanced->setEnabled(AppConfig::instance().get<bool>(AppConfig::SpectrumEnabled));

    ui->sa_type->setCurrentIndex(AppConfig::instance().get<int>(AppConfig::SpectrumTheme));
	ui->sa_bands->setValue(bands);
	ui->sa_minfreq->setValue(minfreq);
	ui->sa_maxfreq->setValue(maxfreq);
    ui->sa_grid->setChecked(AppConfig::instance().get<bool>(AppConfig::SpectrumGrid));
	ui->sa_refresh->setValue(refresh);
    ui->sa_multi->setValue(multiplier);*/

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
