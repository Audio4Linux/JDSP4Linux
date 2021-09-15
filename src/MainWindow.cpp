#ifdef USE_PULSEAUDIO
#include <PulseAudioService.h>
#else
#include <PipewireAudioService.h>
#endif

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "config/AppConfig.h"
#include "config/ConfigContainer.h"
#include "config/ConfigIO.h"
#include "config/DspConfig.h"
#include "data/EelParser.h"
#include "data/QJsonTableModel.h"
#include "data/VersionContainer.h"
#include "interface/dialog/AutoEqSelector.h"
#include "interface/dialog/PresetDialog.h"
#include "interface/event/EventFilter.h"
#include "interface/event/ScrollFilter.h"
#include "interface/fragment/FirstLaunchWizard.h"
#include "interface/fragment/SettingsFragment.h"
#include "interface/fragment/StatusFragment.h"
#include "interface/LiquidEqualizerWidget.h"
#include "interface/QMessageOverlay.h"
#include "interface/TrayIcon.h"
#include "utils/Common.h"
#include "utils/dbus/ClientProxy.h"
#include "utils/dbus/ServerAdaptor.h"
#include "utils/Log.h"
#include "utils/MathFunctions.h"
#include "utils/OverlayMsgProxy.h"
#include "utils/StyleHelper.h"

//#include <audiostreamengine.h>
#include <Animation/Animation.h>
#include <eeleditor.h>
//#include <spectrograph.h>

#include <QDesktopServices>
#include <QButtonGroup>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QWhatsThis>

#define STR_(x) #x
#define STR(x) STR_(x)

using namespace std;

MainWindow::MainWindow(QString  exepath,
                       bool     statupInTray,
                       bool     allowMultipleInst,
                       QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// Prepare logger
	{
        Log::clear();
#ifdef USE_PULSEAUDIO
        QString flavor = " (Pulseaudio flavor)";
#else
        QString flavor = " (Pipewire flavor)";
#endif
        Log::information("Application version: " + QString(STR(APP_VERSION)) + flavor);
	}

    // Check if another instance is already running and switch to it if that's the case
    {
        new GuiAdaptor(this);

        QDBusConnection connection                    = QDBusConnection::sessionBus();
        bool            aboutToQuit                   = false;
        bool            serviceRegistrationSuccessful = connection.registerObject("/Gui", this);
        bool            objectRegistrationSuccessful  = connection.registerService("me.timschneeberger.jdsp4linux.Gui");

        if (serviceRegistrationSuccessful && objectRegistrationSuccessful)
        {
            Log::information("MainWindow::ctor: DBus service registration successful");
        }
        else
        {
            Log::warning("DBus service registration failed. Name already aquired by other instance");

            if (!allowMultipleInst)
            {
                Log::information("Attempting to switch to this instance...");
                auto m_dbInterface = new cf::thebone::jdsp4linux::Gui("me.timschneeberger.jdsp4linux.Gui", "/Gui",
                                                                      QDBusConnection::sessionBus(), this);

                if (!m_dbInterface->isValid())
                {
                    Log::error("Critical: Unable to connect to other DBus instance. Continuing anyway...");
                }
                else
                {
                    QDBusPendingReply<> msg = m_dbInterface->raiseWindow();

                    if (msg.isError() || msg.isValid())
                    {
                        Log::error("Critical: Other DBus instance returned (invalid) error message. Continuing anyway...");
                    }
                    else
                    {
                        aboutToQuit = true;
                        Log::information("Success! Waiting for event loop to exit...");
                        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
                    }
                }
            }
        }

        // Cancel constructor if quitting soon
        if (aboutToQuit)
        {
            return;
        }
    }

    // Prepare audio subsystem
    {
        Log::information("============ Initializing audio service ============");
#ifdef USE_PULSEAUDIO
        Log::information("Compiled with PulseAudio support.");
        Log::information("This application flavor does not support PipeWire or its PulseAudio compatibility mode.");
        Log::information("If you want to use this application with PipeWire, you need to recompile this app with proper support enabled.");
        Log::information("Refer to the README for more detailed information.");
        Log::information("");
        audioService = new PulseAudioService();
#else
        Log::information("Compiled with PipeWire support.");
        Log::information("This application flavor does not support PulseAudio.");
        Log::information("If you want to use this application with PulseAudio, you need to recompile this app with proper support enabled.");
        Log::information("Refer to the README for more detailed information.");
        Log::information("");
        audioService = new PipewireAudioService();
#endif
        connect(&DspConfig::instance(), &DspConfig::updated, audioService, &IAudioService::update);
    }

	// Prepare base UI
	{
        Log::information("============ Initializing user interface ============");

        this->setWindowIcon(QIcon::fromTheme("jamesdsp-tray", QIcon(":/icons/icon.png")));

		ui->eq_widget->setBands(PresetProvider::EQ::defaultPreset(), false);
		ui->eq_dyn_widget->setSidebarHidden(true);
		ui->eq_dyn_widget->set15BandFreeMode(true);

		ui->liveprog_reset->hide();

		ConfigContainer pref;
		pref.setValue("scrollX", 160);
		pref.setValue("scrollY", 311);
		pref.setValue("zoomX",   0.561);
		pref.setValue("zoomY",   1.651);
		ui->eq_dyn_widget->loadPreferences(pref.getConfigMap());

		QButtonGroup eq_mode;
		eq_mode.addButton(ui->eq_r_fixed);
		eq_mode.addButton(ui->eq_r_flex);

		refreshTick = new QTimer(this);
		connect(refreshTick, &QTimer::timeout, this, &MainWindow::fireTimerSignal);
		refreshTick->start(1000);
	}

	// Allocate pointers and init important variables
	{
        AppConfig::instance().set(AppConfig::ExecutablePath, exepath);

		m_startupInTraySwitch = statupInTray;

		m_stylehelper         = new StyleHelper(this);
		m_eelEditor           = new EELEditor(this);

		_eelparser            = new EELParser();

        connect(audioService, &IAudioService::eelCompilationStarted, m_eelEditor, &EELEditor::onCompilerStarted);
        connect(audioService, &IAudioService::eelCompilationFinished, m_eelEditor, &EELEditor::onCompilerFinished);
        connect(audioService, &IAudioService::eelOutputReceived, m_eelEditor, &EELEditor::onConsoleOutputReceived);
        connect(m_eelEditor, &EELEditor::runCode, [this](QString path){
            bool isSameFile = path == activeliveprog;
            if (QFileInfo::exists(path) && QFileInfo(path).isFile())
            {
                activeliveprog = path;
            }
            else
            {
                QMessageBox::critical(m_eelEditor, "Cannot execute",
                                      QString("The current EEL file (at '%1') does not exist anymore on the filesystem. Please reopen the file manually.").arg(path));
                return;
            }

            setLiveprogSelection(activeliveprog);

            applyConfig();

            if(isSameFile)
            {
                audioService->reloadLiveprog();
            }
        });
    }

	// Prepare tray icon
	{
		trayIcon = new TrayIcon(this);
		connect(trayIcon,               &TrayIcon::iconActivated,    this,     &MainWindow::trayIconActivated);
		connect(trayIcon,               &TrayIcon::loadReverbPreset, [this](const QString &preset)
		{
            if(preset == "off"){
                ui->reverb->setChecked(false);
                applyConfig();
                return;
            }

            ui->reverb->setChecked(true);
			ui->roompresets->setCurrentText(preset);
			reverbPresetSelectionUpdated();
		});
        connect(trayIcon, &TrayIcon::restart, this, &MainWindow::restart);
		connect(trayIcon, &TrayIcon::loadEqPreset, [this](const QString &preset)
		{
            ui->enable_eq->setChecked(true);

			if (preset == "Default")
			{
			    resetEQ();
			}
			else
			{
			    ui->eqpreset->setCurrentText(preset);
			    eqPresetSelectionUpdated();
			}
		});
		connect(trayIcon, &TrayIcon::loadCrossfeedPreset, [this](int preset)
		{
            if(preset == -1)
            {
                ui->bs2b->setChecked(false);
                applyConfig();
                return;
            }

            auto name = PresetProvider::BS2B::reverseLookup(preset);
            ui->crossfeed_mode->setCurrentText(name);
            ui->bs2b->setChecked(true);
            bs2bPresetSelectionUpdated();
		});
		connect(trayIcon, &TrayIcon::loadIrs, [this](const QString &irs)
		{
			activeirs = irs;
            ui->conv_enable->setChecked(true);
			updateIrsSelection();
			applyConfig();
		});
		connect(trayIcon, &TrayIcon::loadPreset, [this](const QString &preset)
		{
            loadPresetFile(preset);
		});
		connect(trayIcon, &TrayIcon::changeDisableFx, ui->disableFX, &QPushButton::setChecked);
		connect(trayIcon, &TrayIcon::changeDisableFx, this,          &MainWindow::applyConfig);

        trayIcon->setTrayVisible(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled) || m_startupInTraySwitch);
	}

	// Load config and initialize less important stuff
	{
        //initializeSpectrum();

		connect(&DspConfig::instance(), &DspConfig::configBuffered, this, &MainWindow::loadConfig);
		DspConfig::instance().load();

		preset_dlg   = new PresetDialog(this);
        connect(preset_dlg, &PresetDialog::wantsToWriteConfig, this, &MainWindow::applyConfig);

        settings_dlg = new SettingsFragment(trayIcon, audioService, this);
		connect(settings_dlg, &SettingsFragment::launchSetupWizard,       this, &MainWindow::launchFirstRunSetup);
		connect(settings_dlg, &SettingsFragment::requestEelScriptExtract, this, &MainWindow::extractDefaultEelScripts);

		settingsFragmentHost = new QFrame(this);
		settingsHostLayout   = new QVBoxLayout(settingsFragmentHost);

		settingsHostLayout->addWidget(settings_dlg);
		settingsFragmentHost->setProperty("menu", false);
		settingsFragmentHost->hide();
		settingsFragmentHost->setAutoFillBackground(true);

		connect(settings_dlg, &SettingsFragment::closeClicked, this, [ = ]() {
			settingsFragmentHost->update();
			settingsFragmentHost->repaint();
			WAF::Animation::sideSlideOut(settingsFragmentHost, WAF::BottomSide);
		});
		connect(settings_dlg, &SettingsFragment::reopenSettings, this, [ = ]() {
			settingsFragmentHost->update();
			settingsFragmentHost->repaint();
			WAF::Animation::sideSlideOut(settingsFragmentHost, WAF::BottomSide);
			QTimer::singleShot(500, this, [ = ] {
				WAF::Animation::sideSlideIn(settingsFragmentHost, WAF::BottomSide);
			});
        });

        presetFragmentHost = new QFrame(this);
        presetHostLayout   = new QVBoxLayout(presetFragmentHost);

        presetHostLayout->addWidget(preset_dlg);
        preset_dlg->show();
        presetFragmentHost->setProperty("menu", false);
        presetFragmentHost->hide();
        presetFragmentHost->setAutoFillBackground(true);

        connect(preset_dlg, &PresetDialog::closePressed, this, [ = ]() {
            presetFragmentHost->update();
            presetFragmentHost->repaint();
            WAF::Animation::sideSlideOut(presetFragmentHost, WAF::LeftSide);
        });
    }

	// Init 3-dot menu button
	{
		QMenu *menu = new QMenu();
		spectrum = new QAction("Reload spectrum", this);
		connect(spectrum, &QAction::triggered, this, &MainWindow::restartSpectrum);
        menu->addAction(tr("Reload JamesDSP"),   this, SLOT(restart()));
		menu->addAction(tr("Reset to defaults"), this, SLOT(reset()));
        //menu->addAction(spectrum);
		menu->addAction(tr("Driver status"),     this, [this]()
		{
            StatusFragment *sd      = new StatusFragment(audioService->status());
			QWidget *host           = new QWidget(this);
			host->setProperty("menu", false);
			QVBoxLayout *hostLayout = new QVBoxLayout(host);
			hostLayout->addWidget(sd);
			host->hide();
			host->setAutoFillBackground(true);
			connect(sd, &StatusFragment::closePressed, this, [host]() {
				WAF::Animation::sideSlideOut(host, WAF::BottomSide);
			});
			WAF::Animation::sideSlideIn(host, WAF::BottomSide);
		});
		menu->addAction(tr("Load from file"), this, SLOT(loadExternalFile()));
		menu->addAction(tr("Save to file"),   this, SLOT(saveExternalFile()));
		menu->addAction(tr("What's this..."), this, []()
		{
			QWhatsThis::enterWhatsThisMode();
		});
		ui->toolButton->setMenu(menu);
	}

	// Prepare styles
	{
		m_stylehelper->SetStyle();
		ui->eq_widget->setAccentColor(palette().highlight().color());
	}

	// Extract default EEL files if missing
	{
        if (AppConfig::instance().get<bool>(AppConfig::LiveprogAutoExtract))
		{
			extractDefaultEelScripts(false, false);
		}
	}

	// Load DDC/IRS/Liveprog lists
	{
		// Reload DDC selection
		reloadDDCDB();
		updateDDCSelection();

		// Reload IRS lists
		updateIrsSelection();

		// Reload Liveprog selection
		if (!QFile(activeliveprog).exists())
		{
			ui->liveprog_dirpath->setText(AppConfig::instance().getLiveprogPath());
			reloadLiveprog();
		}
		else
		{
            QDir d2 = QFileInfo(activeliveprog).absoluteDir();
            ui->liveprog_dirpath->setText(d2.absolutePath());
            reloadLiveprog();
		}

	}

	// Populate preset lists
	{
		for (const auto &preset : PresetProvider::EQ::EQ_LOOKUP_TABLE().keys())
		{
			ui->eqpreset->addItem(preset);
		}
	}

	// Connect UI signals
	{
		connectActions();
	}

	// Connect non-UI signals (DBus/ACW/...)
	{
        connect(&AppConfig::instance(), &AppConfig::themeChanged, this, [this]()
		{
			m_stylehelper->SetStyle();
			ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
			ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
			ui->tabbar->redrawTabBar();
			ui->eq_widget->setAccentColor(palette().highlight().color());
			spectrumReloadSignalQueued = true;
		});

        connect(&AppConfig::instance(), &AppConfig::updated, this, [this](const AppConfig::Key& key, const QVariant& value)
		{
            switch(key)
            {
                case AppConfig::EqualizerShowHandles:
                    ui->eq_widget->setAlwaysDrawHandles(value.toBool());
                    break;
                case AppConfig::TrayIconEnabled:
                    trayIcon->setTrayVisible(value.toBool());
                    break;
                default:
                    break;
            }

		});
	}

	// Lateinit less important UI stuff and setup tabbar
	{
        //toggleSpectrum(AppConfig::instance().get<bool>(AppConfig::SpectrumEnabled), true);
		restoreGraphicEQView();
        ui->eq_widget->setAlwaysDrawHandles(AppConfig::instance().get<bool>(AppConfig::EqualizerShowHandles));

		ui->tabbar->setAnimatePageChange(true);
		ui->tabbar->setCustomStackWidget(ui->tabhost);
		ui->tabbar->setDetachCustomStackedWidget(true);
		ui->tabbar->addPage("Bass/Misc");
		ui->tabbar->addPage("Sound Positioning");
		ui->tabbar->addPage("Reverb");
		ui->tabbar->addPage("Equalizer");
		ui->tabbar->addPage("Convolver");
		ui->tabbar->addPage("DDC");
		ui->tabbar->addPage("Liveprog");
		ui->tabbar->addPage("Graphic EQ");
		ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
		ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7,QWidget#tabHostPage8,QWidget#tabHostPage9{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
		ui->tabbar->redrawTabBar();
	}

	// Handle first launch and diagnostic checks
	{
        if (!AppConfig::instance().get<bool>(AppConfig::SetupDone))
		{
			launchFirstRunSetup();
		}
	}

    Log::information("MainWindow::ctor: UI initialized");
}

MainWindow::~MainWindow()
{
    if(audioService != nullptr)
    {
        delete audioService;
    }
	delete ui;
}

// Spectrum
void MainWindow::setSpectrumVisibility(bool v)
{
    /*m_spectrograph->setVisible(v);

	if (v)
	{
		this->findChild<QFrame*>("analysisLayout_spectrum")->setFrameShape(QFrame::StyledPanel);
	}
	else
	{
		this->findChild<QFrame*>("analysisLayout_spectrum")->setFrameShape(QFrame::NoFrame);
    }*/
}

void MainWindow::initializeSpectrum()
{
    /*m_spectrograph = new Spectrograph(this);
	m_audioengine  = new AudioStreamEngine(this);

    int refresh = AppConfig::instance().get<int>(AppConfig::SpectrumRefresh);

	if (refresh == 0)
	{
		refresh = 20;
	}

	if (refresh < 10)
	{
		refresh = 10;
	}
	else if (refresh > 500)
	{
		refresh = 500;
	}

	m_audioengine->setNotifyIntervalMs(refresh);

	analysisLayout.reset(new QFrame());
	analysisLayout->setObjectName("analysisLayout_spectrum");
	analysisLayout->setFrameShape(QFrame::Shape::StyledPanel);
	analysisLayout->setLayout(new QHBoxLayout);
	analysisLayout->layout()->setMargin(0);
	analysisLayout->layout()->addWidget(m_spectrograph);

	auto buttonbox = ui->centralWidget->layout()->takeAt(ui->centralWidget->layout()->count() - 1);
	ui->centralWidget->layout()->addWidget(analysisLayout.data());
	ui->centralWidget->layout()->addItem(buttonbox);
	analysisLayout.take();

	setSpectrumVisibility(false);

    connect(&AppConfig::instance(), &AppConfig::spectrumChanged, this, [this](bool needReload) {
        toggleSpectrum(AppConfig::instance().get<bool>(AppConfig::SpectrumEnabled), true);
        if(needReload)
            restartSpectrum();
    });*/
}

void MainWindow::restartSpectrum()
{
    //toggleSpectrum(false,                                     false);
    //toggleSpectrum(AppConfig::instance().get<bool>(AppConfig::SpectrumEnabled), false);
}

void MainWindow::refreshSpectrumParameters()
{
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

	QColor outline;

	if (palette().window().style() == Qt::TexturePattern)
	{
		outline = QColor(0, 0, 0, 160);
	}
	else
	{
		outline = palette().window().color().lighter(140);
	}

	m_spectrograph->setTheme(palette().window().color().lighter(),
	                         palette().highlight().color(),
	                         palette().text().color(),
	                         outline.lighter(108),
                             AppConfig::instance().get<bool>(AppConfig::SpectrumGrid),
                             (Spectrograph::Mode) AppConfig::instance().get<int>(AppConfig::SpectrumTheme));

	m_spectrograph->setParams(bands, minfreq, maxfreq);
	m_audioengine->setNotifyIntervalMs(refresh);
    m_audioengine->setMultiplier(multiplier);*/
}

void MainWindow::toggleSpectrum(bool on,
                                bool ctrl_visibility)
{
    /*refreshSpectrumParameters();

	if (ctrl_visibility)
	{
		spectrum->setVisible(on);
	}

	if (on && (!m_spectrograph->isVisible() || !ctrl_visibility))
	{
		if (ctrl_visibility)
		{
			setSpectrumVisibility(true);
			this->setFixedSize(this->width(), this->height() + m_spectrograph->size().height());
		}

		QAudioDeviceInfo in;

		for (auto item : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
		{
			if (item.deviceName() == AppConfig::instance().getSpectrumInput())
			{
				in = item;
			}
		}

		m_audioengine->setAudioInputDevice(in);
		m_audioengine->initializeRecord();
		m_audioengine->startRecording();

		connect(m_audioengine, static_cast<void (AudioStreamEngine::*)(QAudio::Mode, QAudio::State)>(&AudioStreamEngine::stateChanged),
		        this, [this](QAudio::Mode mode, QAudio::State state) {
			Q_UNUSED(mode);

			if (QAudio::ActiveState != state && QAudio::SuspendedState != state)
			{
			    m_spectrograph->reset();
			}
		});

		connect(m_audioengine, static_cast<void (AudioStreamEngine::*)(qint64, qint64, const FrequencySpectrum &)>(&AudioStreamEngine::spectrumChanged),
		        this, [this](qint64, qint64, const FrequencySpectrum &spectrum) {
			m_spectrograph->spectrumChanged(spectrum);
		});
	}
	else if (!on && (m_spectrograph->isVisible() || !ctrl_visibility))
	{
		if (ctrl_visibility)
		{
			setSpectrumVisibility(false);
			this->setFixedSize(this->width(), this->height() - m_spectrograph->size().height());
		}

		m_spectrograph->reset();
		m_audioengine->reset();
    }*/
}

void MainWindow::fireTimerSignal()
{
    if (spectrumReloadSignalQueued)
	{
		restartSpectrum();
    }

	spectrumReloadSignalQueued = false;
}

// Overrides
void MainWindow::closeEvent(QCloseEvent *event)
{
	saveGraphicEQView();
#ifdef Q_OS_OSX

	if (!event->spontaneous() || !isVisible())
	{
		return;
	}

#endif

	if (trayIcon->isVisible())
	{
		hide();
		event->ignore();
	}
}

// Systray
void MainWindow::raiseWindow()
{
	/*
	 * NOTE: Raising the window does not always work!
	 *
	 * KDE users can disable 'Focus Stealing Prevention'
	 * in the Window Behavior section (system settings)
	 * as a workaround.
	 */
	show();
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	raise();
	activateWindow();
}

void MainWindow::trayIconActivated()
{
	setVisible(!this->isVisible());

	if (isVisible())
	{
		raiseWindow();
	}

	// Hide tray icon if disabled and MainWin is visible (for cmdline force switch)
    if (!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled) && this->isVisible())
	{
		trayIcon->setTrayVisible(false);
	}
}

// ---Dialogs/Buttons
void MainWindow::dialogHandler()
{
	if (sender() == ui->set)
	{
		WAF::Animation::sideSlideIn(settingsFragmentHost, WAF::BottomSide);
	}
	else if (sender() == ui->cpreset)
	{
        WAF::Animation::sideSlideIn(presetFragmentHost, WAF::LeftSide);
	}
}

void MainWindow::reset()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("Reset Configuration"), tr("Are you sure?"),
	                              QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes)
	{
		DspConfig::instance().loadDefault();
	}
}

void MainWindow::disableFx()
{
	trayIcon->changedDisableFx(ui->disableFX->isChecked());

	// Apply instantly
	if (!lockapply)
	{
		applyConfig();
	}
}

// ---Reloader
void MainWindow::onUpdate()
{
    if (!lockapply)
	{
		applyConfig();
	}
}

void MainWindow::restart()
{
    audioService->reloadService();
    DspConfig::instance().commit();

    // TODO
	spectrumReloadSignalQueued = true;
}

// ---User preset management
void MainWindow::loadPresetFile(const QString &filename)
{
	const QString &src  = filename;
	const QString  dest = AppConfig::instance().getDspConfPath();

	if (QFile::exists(dest))
	{
		QFile::remove(dest);
	}

	QFile::copy(src, dest);
    Log::debug("MainWindow::loadPresetFile: Loading from " + filename);
	DspConfig::instance().load();
}

void MainWindow::savePresetFile(const QString &filename)
{
	const QString  src  = AppConfig::instance().getDspConfPath();
	const QString &dest = filename;

	if (QFile::exists(dest))
	{
		QFile::remove(dest);
	}

	QFile::copy(src, dest);
    Log::debug("MainWindow::savePresetFile: Saving to " + filename);
}

void MainWindow::loadExternalFile()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load custom audio.conf"), "", "*.conf");

	if (filename == "")
	{
		return;
	}

	loadPresetFile(filename);
}

void MainWindow::saveExternalFile()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save current audio.conf"), "", "*.conf");

	if (filename == "")
	{
		return;
	}

	QFileInfo fi(filename);
	QString   ext = fi.suffix();

	if (ext != "conf")
	{
		filename.append(".conf");
	}

    applyConfig();
	savePresetFile(filename);
}

// ---Config IO
void MainWindow::loadConfig()
{
	lockapply = true;

    auto dsp = &DspConfig::instance();
    dsp->get<bool>(DspConfig::master_enable);

    trayIcon->changedDisableFx(!DspConfig::instance().get<bool>(DspConfig::master_enable));
    ui->disableFX->setChecked(!DspConfig::instance().get<bool>(DspConfig::master_enable));

    ui->analog->setChecked(DspConfig::instance().get<bool>(DspConfig::tube_enable));
    ui->analog_tubedrive->setValueA(DspConfig::instance().get<int>(DspConfig::tube_pregain));

    ui->bassboost->setChecked(DspConfig::instance().get<bool>(DspConfig::bass_enable));
    ui->bass_maxgain->setValueA(DspConfig::instance().get<int>(DspConfig::bass_maxgain));

    ui->reverb->setChecked(DspConfig::instance().get<bool>(DspConfig::reverb_enable));
    ui->rev_osf->setValueA(DspConfig::instance().get<int>(DspConfig::reverb_osf));
    ui->rev_era->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_reflection_amount));
    ui->rev_erw->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_reflection_width));
    ui->rev_erf->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_reflection_factor));
    ui->rev_finaldry->setValueA(10 * DspConfig::instance().get<float>(DspConfig::reverb_finaldry));
    ui->rev_finalwet->setValueA(10 * DspConfig::instance().get<float>(DspConfig::reverb_finalwet));
    ui->rev_width->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_width));
    ui->rev_wet->setValueA(10 * DspConfig::instance().get<float>(DspConfig::reverb_wet));
    ui->rev_bass->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_bassboost));
    ui->rev_spin->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_lfo_spin));
    ui->rev_wander->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_lfo_wander));
    ui->rev_decay->setValueA(100 * DspConfig::instance().get<float>(DspConfig::reverb_decay));
    ui->rev_delay->setValueA(10 * DspConfig::instance().get<float>(DspConfig::reverb_delay));
    ui->rev_lci->setValueA(DspConfig::instance().get<int>(DspConfig::reverb_lpf_input));
    ui->rev_lcb->setValueA(DspConfig::instance().get<int>(DspConfig::reverb_lpf_bass));
    ui->rev_lcd->setValueA(DspConfig::instance().get<int>(DspConfig::reverb_lpf_damp));
    ui->rev_lco->setValueA(DspConfig::instance().get<int>(DspConfig::reverb_lpf_output));

    ui->stereowidener->setChecked(DspConfig::instance().get<bool>(DspConfig::stereowide_enable));
    ui->stereowide_level->setValueA(DspConfig::instance().get<int>(DspConfig::stereowide_level));

    ui->bs2b->setChecked(DspConfig::instance().get<bool>(DspConfig::crossfeed_enable));
    int bs2bMode = DspConfig::instance().get<int>(DspConfig::crossfeed_mode);
    ui->crossfeed_mode->setCurrentText(PresetProvider::BS2B::reverseLookup(bs2bMode));
    ui->bs2b_feed->setValueA(DspConfig::instance().get<int>(DspConfig::crossfeed_bs2b_feed));
    ui->bs2b_fcut->setValueA(DspConfig::instance().get<int>(DspConfig::crossfeed_bs2b_fcut));
    ui->bs2b_custom_box->setEnabled(bs2bMode == 99);

    ui->enable_comp->setChecked(DspConfig::instance().get<bool>(DspConfig::compression_enable));
    ui->comp_maxattack->setValueA(DspConfig::instance().get<int>(DspConfig::compression_maxatk));
    ui->comp_maxrelease->setValueA(DspConfig::instance().get<int>(DspConfig::compression_maxrel));
    ui->comp_aggressiveness->setValueA(DspConfig::instance().get<int>(DspConfig::compression_aggressiveness));

    ui->limthreshold->setValueA(DspConfig::instance().get<int>(DspConfig::master_limthreshold));
    ui->limrelease->setValueA(DspConfig::instance().get<int>(DspConfig::master_limrelease));
    ui->postgain->setValueA(DspConfig::instance().get<int>(DspConfig::master_postgain));

    ui->graphicEq->chk_enable->setChecked(DspConfig::instance().get<bool>(DspConfig::graphiceq_enable));
    ui->graphicEq->load(chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::graphiceq_param)));

    ui->ddc_enable->setChecked(DspConfig::instance().get<bool>(DspConfig::ddc_enable));
    activeddc      = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::ddc_file));

    ui->liveprog_enable->setChecked(DspConfig::instance().get<bool>(DspConfig::liveprog_enable));
    activeliveprog = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::liveprog_file));

    ui->conv_enable->setChecked(DspConfig::instance().get<bool>(DspConfig::convolver_enable));
    ui->conv_ir_opt->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::convolver_optimization_mode));
    activeirs                 = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::convolver_file));
    irAdvancedWaveformEditing = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::convolver_waveform_edit));

    ui->enable_eq->setChecked(DspConfig::instance().get<bool>(DspConfig::tone_enable));
    ui->eqinterpolator->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::tone_interpolation));
    ui->eqfiltertype->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::tone_filtertype));

	// Parse EQ String to QMap
    QString rawEqString = chopFirstLastChar(DspConfig::instance().get<QString>(DspConfig::tone_eq));
	bool    isOldFormat = rawEqString.split(";").count() == 15;

	if (isOldFormat)
	{
		rawEqString = "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;"
		              + rawEqString;
	}

	QVector<float> rawEqData;

	for (auto val : rawEqString.split(";"))
	{
		if (!val.isEmpty())
		{
			if (isOldFormat)
			{
				rawEqData.push_back(val.toFloat() / 100.f);
			}
			else
			{
				rawEqData.push_back(val.toFloat());
			}
		}
		else
		{
			rawEqData.push_back(0.f);
		}
	}

	QMap<float, float> eqData;
	QVector<double>    dbData;

	for (int i = 0; i < rawEqData.size(); i++)
	{
		if (i <= 14)
		{
			if ((i + 15) < rawEqData.size())
			{
				eqData[rawEqData.at(i)] = rawEqData.at(i + 15);
			}
			else
			{
				eqData[rawEqData.at(i)] = 0.0;
			}
		}
		else
		{
			dbData.push_back(rawEqData.at(i));
		}
	}

	// Decide if fixed or flexible EQ should be enabled
	if (rawEqString.contains("25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0"))
	{
		// Use fixed 15-band EQ
		setEqMode(0);

		int  it               = 0;
		bool eqReloadRequired = false;

		for (auto cur_data : ui->eq_widget->getBands())
		{
			if (it >= rawEqData.count())
			{
				break;
			}

			bool equal = isApproximatelyEqual<float>(cur_data, dbData.at(it));

			if (eqReloadRequired == false)
			{
				eqReloadRequired = !equal;
			}

			it++;
		}

		if (eqReloadRequired)
		{
			ui->eq_widget->setBands(dbData, false);
		}
	}
	else
	{
		// Use flexible 15-band EQ
		setEqMode(1);
		ui->eq_dyn_widget->loadMap(eqData);
	}

	updateEqStringFromWidget();
	updateAllUnitLabels();
	updateIrsSelection();
	updateDDCSelection();
    reloadLiveprog();

	lockapply = false;
}

void MainWindow::applyConfig()
{
    if(lockapply)
    {
        return;
    }

    DspConfig::instance().set(DspConfig::master_enable,              QVariant(!ui->disableFX->isChecked()));

    DspConfig::instance().set(DspConfig::tube_enable,                QVariant(ui->analog->isChecked()));
    DspConfig::instance().set(DspConfig::tube_pregain,               QVariant(ui->analog_tubedrive->valueA()));
    DspConfig::instance().set(DspConfig::master_limthreshold,        QVariant(ui->limthreshold->valueA()));
    DspConfig::instance().set(DspConfig::master_limrelease,          QVariant(ui->limrelease->valueA()));
    DspConfig::instance().set(DspConfig::master_postgain,            QVariant(ui->postgain->valueA()));

    DspConfig::instance().set(DspConfig::ddc_enable,                 QVariant(ui->ddc_enable->isChecked()));
    DspConfig::instance().set(DspConfig::ddc_file,                   QVariant("\"" + activeddc + "\""));

    DspConfig::instance().set(DspConfig::liveprog_enable,            QVariant(ui->liveprog_enable->isChecked()));
    DspConfig::instance().set(DspConfig::liveprog_file,              QVariant("\"" + activeliveprog + "\""));

    DspConfig::instance().set(DspConfig::convolver_enable,           QVariant(ui->conv_enable->isChecked()));
    DspConfig::instance().set(DspConfig::convolver_optimization_mode,QVariant(ui->conv_ir_opt->currentIndex()));
    DspConfig::instance().set(DspConfig::convolver_file,             QVariant("\"" + activeirs + "\""));
    DspConfig::instance().set(DspConfig::convolver_waveform_edit,    QVariant("\"" + irAdvancedWaveformEditing + "\""));

    DspConfig::instance().set(DspConfig::compression_enable,         QVariant(ui->enable_comp->isChecked()));
    DspConfig::instance().set(DspConfig::compression_maxatk,         QVariant(ui->comp_maxattack->valueA()));
    DspConfig::instance().set(DspConfig::compression_maxrel,         QVariant(ui->comp_maxrelease->valueA()));
    DspConfig::instance().set(DspConfig::compression_aggressiveness, QVariant(ui->comp_aggressiveness->valueA()));

    DspConfig::instance().set(DspConfig::tone_enable,                QVariant(ui->enable_eq->isChecked()));
    DspConfig::instance().set(DspConfig::tone_filtertype,            QVariant(ui->eqfiltertype->currentIndex()));
    DspConfig::instance().set(DspConfig::tone_interpolation,         QVariant(ui->eqinterpolator->currentIndex()));

	if (ui->eq_r_fixed->isChecked())
	{
		QVector<double> eqBands     = ui->eq_widget->getBands();
		QString         rawEqString = "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;";
		int             counter     = 0;

		for (auto band : eqBands)
		{
			rawEqString.append(QString::number(band));

			if (counter < 14)
			{
				rawEqString.append(';');
			}

			counter++;
		}

        DspConfig::instance().set(DspConfig::tone_eq, QVariant("\"" + rawEqString + "\""));
	}
	else
	{
		QString rawEqString;
		ui->eq_dyn_widget->storeCsv(rawEqString);
        DspConfig::instance().set(DspConfig::tone_eq, QVariant("\"" + rawEqString + "\""));
	}

    DspConfig::instance().set(DspConfig::bass_enable,               QVariant(ui->bassboost->isChecked()));
    DspConfig::instance().set(DspConfig::bass_maxgain,              QVariant(ui->bass_maxgain->valueA()));

    DspConfig::instance().set(DspConfig::stereowide_enable,         QVariant(ui->stereowidener->isChecked()));
    DspConfig::instance().set(DspConfig::stereowide_level,          QVariant(ui->stereowide_level->valueA()));

    DspConfig::instance().set(DspConfig::crossfeed_enable,          QVariant(ui->bs2b->isChecked()));
    DspConfig::instance().set(DspConfig::crossfeed_mode,            QVariant(PresetProvider::BS2B::lookupPreset(ui->crossfeed_mode->currentText())));
    DspConfig::instance().set(DspConfig::crossfeed_bs2b_feed,       QVariant(ui->bs2b_feed->valueA()));
    DspConfig::instance().set(DspConfig::crossfeed_bs2b_fcut,       QVariant(ui->bs2b_fcut->valueA()));

    DspConfig::instance().set(DspConfig::reverb_enable,             QVariant(ui->reverb->isChecked()));
    DspConfig::instance().set(DspConfig::reverb_osf,                QVariant(ui->rev_osf->valueA()));
    DspConfig::instance().set(DspConfig::reverb_lpf_input,          QVariant(ui->rev_lci->valueA()));
    DspConfig::instance().set(DspConfig::reverb_lpf_bass,           QVariant(ui->rev_lcb->valueA()));
    DspConfig::instance().set(DspConfig::reverb_lpf_damp,           QVariant(ui->rev_lcd->valueA()));
    DspConfig::instance().set(DspConfig::reverb_lpf_output,         QVariant(ui->rev_lco->valueA()));
    DspConfig::instance().set(DspConfig::reverb_reflection_amount,  QVariant(ui->rev_era->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_reflection_width,   QVariant(ui->rev_erw->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_reflection_factor,  QVariant(ui->rev_erf->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_finaldry,           QVariant(ui->rev_finaldry->valueA() / 10.0f));
    DspConfig::instance().set(DspConfig::reverb_finalwet,           QVariant(ui->rev_finalwet->valueA() / 10.0f));
    DspConfig::instance().set(DspConfig::reverb_width,              QVariant(ui->rev_width->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_wet,                QVariant(ui->rev_wet->valueA() / 10.0f));
    DspConfig::instance().set(DspConfig::reverb_bassboost,          QVariant(ui->rev_bass->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_lfo_spin,           QVariant(ui->rev_spin->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_lfo_wander,         QVariant(ui->rev_wander->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_decay,              QVariant(ui->rev_decay->valueA() / 100.0f));
    DspConfig::instance().set(DspConfig::reverb_delay,              QVariant(ui->rev_delay->valueA() / 10.0f));

    DspConfig::instance().set(DspConfig::graphiceq_enable,          QVariant(ui->graphicEq->chk_enable->isChecked()));
	QString streq;
	ui->graphicEq->store(streq);
    DspConfig::instance().set(DspConfig::graphiceq_param,           QVariant("\"" + streq + "\""));

    DspConfig::instance().commit();
    DspConfig::instance().save();

    saveGraphicEQView();
}

// ---Predefined Presets
void MainWindow::eqPresetSelectionUpdated()
{
    if (ui->eqpreset->currentText() == "Custom" || lockapply)
	{
		return;
	}

	auto preset = PresetProvider::EQ::lookupPreset(ui->eqpreset->currentText());

	if (preset.size() > 0)
	{
		setEq(preset);
	}
	else
	{
		resetEQ();
	}
}

void MainWindow::bs2bPresetSelectionUpdated()
{
    if (ui->crossfeed_mode->currentText() == "..." || lockapply)
	{
		return;
	}

	const auto index = PresetProvider::BS2B::lookupPreset(ui->crossfeed_mode->currentText());

    lockapply = true;
	switch (index)
	{
		case 0: // BS2B weak
			ui->bs2b_fcut->setValueA(700);
			ui->bs2b_feed->setValueA(60);
			break;
		case 1: // BS2B strong
			ui->bs2b_fcut->setValueA(650);
			ui->bs2b_feed->setValueA(95);
			break;
	}

    ui->bs2b_custom_box->setEnabled(index == 99);
    lockapply = false;

	updateAllUnitLabels();
    onUpdate();
}

void MainWindow::reverbPresetSelectionUpdated()
{
    if (ui->roompresets->currentText() == "..." || lockapply)
	{
		return;
	}

	const auto data = PresetProvider::Reverb::lookupPreset(ui->roompresets->currentIndex());
	lockapply = true;
	ui->rev_osf->setValueA(data.osf);
	ui->rev_era->setValueA((int) (data.p1 * 100));
	ui->rev_finalwet->setValueA((int) (data.p2 * 10));
	ui->rev_finaldry->setValueA((int) (data.p3 * 10));
	ui->rev_erf->setValueA((int) (data.p4 * 100));
	ui->rev_erw->setValueA((int) (data.p5 * 100));
	ui->rev_width->setValueA((int) (data.p6 * 100));
	ui->rev_wet->setValueA((int) (data.p7 * 10));
	ui->rev_wander->setValueA((int) (data.p8 * 100));
	ui->rev_bass->setValueA((int) (data.p9 * 100));
	ui->rev_spin->setValueA((int) (data.p10 * 100));
	ui->rev_lci->setValueA((int) data.p11);
	ui->rev_lcb->setValueA((int) data.p12);
	ui->rev_lcd->setValueA((int) data.p13);
	ui->rev_lco->setValueA((int) data.p14);
	ui->rev_decay->setValueA((int) (data.p15 * 100));
	ui->rev_delay->setValueA((int) (data.p16 * 10));
	updateAllUnitLabels();
	lockapply = false;
    onUpdate();
}

// ---Status
void MainWindow::updateUnitLabel(int      d,
                                 QObject *alt)
{
	if (lockapply && alt == nullptr)
	{
		return;                       // Skip if lockapply-flag is set (when setting presets, ...)
	}

	QObject *obj;

	if (alt == nullptr)
	{
		obj = sender();
	}
	else
	{
		obj = alt;
	}

	QString pre  = "";
	QString post = "";

	if (obj == ui->rev_width)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d) + "%", alt == nullptr);
	}
	else if (obj == ui->bs2b_feed)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 10) + "dB", alt == nullptr);
	}
	else if (obj == ui->analog_tubedrive)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 100) + "dB", alt == nullptr);
	}
	else if (obj == ui->rev_decay)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 100), alt == nullptr);
	}
	else if (obj == ui->rev_delay)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 10) + "ms", alt == nullptr);
	}
	else if (obj == ui->rev_wet || obj == ui->rev_finalwet || obj == ui->rev_finaldry)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 10) + "dB", alt == nullptr);
	}
	else if (obj == ui->rev_era || obj == ui->rev_erf || obj == ui->rev_erw
	         || obj == ui->rev_width || obj == ui->rev_bass || obj == ui->rev_spin
	         || obj == ui->rev_wander)
	{
		updateTooltipLabelUnit(obj, QString::number((double) d / 100), alt == nullptr);
	}
	else if (obj == ui->eqfiltertype || obj == ui->eqinterpolator || obj == ui->conv_ir_opt)
	{
		// Ignore these UI elements
	}
	else if (obj->property("isCustomEELProperty").toBool())
	{
		if (obj->property("handleAsInt").toBool())
		{
			updateTooltipLabelUnit(obj, QString::number((int) d / 100), alt == nullptr);
		}
		else
		{
			updateTooltipLabelUnit(obj, QString::number((double) d / 100), alt == nullptr);
		}
	}
	else
	{
		if (obj == ui->postgain)
		{
			post = "dB";
		}
		else if (obj == ui->comp_maxattack || obj == ui->comp_maxrelease || obj == ui->limrelease)
		{
			post = "ms";
		}
		else if (obj == ui->bass_maxgain)
		{
			post = "dB";
		}
		else if (obj == ui->bs2b_fcut)
		{
			post = "Hz";
		}
		else if (obj == ui->rev_lcb || obj == ui->rev_lcd
		         || obj == ui->rev_lci || obj == ui->rev_lco)
		{
			post = "Hz";
		}
		else if (obj == ui->rev_osf)
		{
			post = "x";
		}

		updateTooltipLabelUnit(obj, pre + QString::number(d) + post, alt == nullptr);
	}
}

void MainWindow::updateTooltipLabelUnit(QObject       *sender,
                                        const QString &text,
                                        bool           viasignal)
{
	QWidget *w = qobject_cast<QWidget*>(sender);
	w->setToolTip(text);

	if (viasignal)
	{
		ui->info->setText(text);
	}
}

void MainWindow::updateAllUnitLabels()
{
	QList<QComboBox*>       comboboxesToBeUpdated({ ui->eqfiltertype, ui->eqinterpolator });

	QList<QAnimatedSlider*> slidersToBeUpdated({
		ui->analog_tubedrive, ui->stereowide_level, ui->bs2b_fcut, ui->bs2b_feed,
		ui->limthreshold, ui->limrelease, ui->comp_maxrelease, ui->comp_maxattack,
		ui->rev_osf, ui->rev_erf, ui->rev_era, ui->rev_erw, ui->rev_lci, ui->rev_lcb, ui->rev_lcd,
		ui->rev_lco, ui->rev_finalwet, ui->rev_finaldry, ui->rev_wet, ui->rev_width, ui->rev_spin, ui->rev_wander, ui->rev_decay,
		ui->rev_delay, ui->rev_bass, ui->postgain, ui->comp_aggressiveness, ui->bass_maxgain
	});

	foreach(auto w, slidersToBeUpdated)
	updateUnitLabel(w->valueA(), w);

	foreach(auto w, comboboxesToBeUpdated)
	updateUnitLabel(w->currentIndex(), w);
}

// ---DDC
void MainWindow::updateDDCSelection()
{
	QString absolute = QFileInfo().absoluteDir().absolutePath();

	if (!QFile(activeddc).exists() || activeddc == AppConfig::instance().getPath("temp.vdc"))
	{
		ui->ddc_dirpath->setText(AppConfig::instance().getDDCPath());
		reloadDDC();

		if (activeddc == AppConfig::instance().getPath("temp.vdc"))
		{
            QString lastId = AppConfig::instance().get<QString>(AppConfig::VdcLastDatabaseId);
			ui->ddcTabs->setCurrentIndex(1);

			if (lastId.isEmpty())
			{
				return;
			}

			if (ui->ddcTable->model() == nullptr)
			{
				return;
			}

			QModelIndexList matches = ui->ddcTable->model()->match(model->index(0, 4), Qt::DisplayRole, lastId, 1);

			foreach(const QModelIndex &index, matches)
			{
				if (lastId == ui->ddcTable->model()->data(ui->ddcTable->model()->index(index.row(), 4)))
				{
					ui->ddcTable->selectRow(index.row());
					ui->ddcTable->scrollTo(index);
					break;
				}
			}

		}
	}
	else
	{
        QDir d2 = QFileInfo(activeddc).absoluteDir();
        ui->ddc_dirpath->setText(d2.absolutePath());
        reloadDDC();
	}
}

void MainWindow::reloadDDC()
{
	lockddcupdate = true;
	QDir        path(ui->ddc_dirpath->text());
	QStringList nameFilter("*.vdc");
	nameFilter.append("*.ddc");
	QStringList files = path.entryList(nameFilter);
	ui->ddc_files->clear();

	if (files.empty())
	{
		QListWidgetItem *placeholder = new QListWidgetItem;
		placeholder->setText("No VDC files found");
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
		ui->ddc_files->addItem(placeholder);
	}
	else
	{
		ui->ddc_files->addItems(files);
	}

	if (ui->ddc_files->count() >= 1)
	{
		for (int i = 0; i < ui->ddc_files->count(); i++)
		{
			if (ui->ddc_files->item(i)->text() == QFileInfo(activeddc).fileName())
			{
				ui->ddc_files->setCurrentRow(i);
				break;
			}
		}
	}

	lockddcupdate = false;
}

void MainWindow::reloadDDCDB()
{
	QJsonTableModel::Header header;
	header.push_back(QJsonTableModel::Heading({
		{ "title", "Company" },   { "index", "Company" }
	}));
	header.push_back(QJsonTableModel::Heading({
		{ "title", "Model" }, { "index", "Model" }
	}));
	header.push_back(QJsonTableModel::Heading({
		{ "title", "SR_44100_Coeffs" }, { "index", "SR_44100_Coeffs" }
	}));
	header.push_back(QJsonTableModel::Heading({
		{ "title", "SR_48000_Coeffs" }, { "index", "SR_48000_Coeffs" }
	}));
	header.push_back(QJsonTableModel::Heading({
		{ "title", "ID" }, { "index", "ID" }
	}));

	model = new QJsonTableModel(header, this);
	ui->ddcTable->setModel(model);

	QFile file(":/assets/DDCData.json");

	if (file.open(QIODevice::ReadOnly))
	{
		QTextStream   instream(&file);
		QJsonDocument jsonDocument = QJsonDocument::fromJson(instream.readAll().toLocal8Bit());
		model->setJson(jsonDocument);
	}

	model->setHeaderData(0, Qt::Horizontal, tr("Company"));
	model->setHeaderData(1, Qt::Horizontal, tr("Model"));
	model->setHeaderData(5, Qt::Horizontal, tr("SR_44100_Coeffs"));
	model->setHeaderData(6, Qt::Horizontal, tr("SR_48000_Coeffs"));
	model->setHeaderData(7, Qt::Horizontal, tr("ID"));

	ui->ddcTable->setModel(model);
	ui->ddcTable->setColumnHidden(2, true);
	ui->ddcTable->setColumnHidden(3, true);
	ui->ddcTable->setColumnHidden(4, true);
	ui->ddcTable->resizeColumnsToContents();
}

// ---IRS
void MainWindow::updateIrsSelection()
{
	reloadIRSFav();
	ui->conv_files_fav->clearSelection();
	ui->conv_files->clearSelection();

	if (activeirs.contains(AppConfig::instance().getPath("irs_favorites")))
	{
		ui->conv_dirpath->setText(AppConfig::instance().getIrsPath());
		reloadIRS();
		ui->convTabs->setCurrentIndex(1);

        if (ui->conv_files_fav->count() >= 1)
        {
            for (int i = 0; i < ui->conv_files_fav->count(); i++)
            {
                if (ui->conv_files_fav->item(i)->text() == QFileInfo(activeirs).fileName())
                {
                    ui->conv_files_fav->setCurrentRow(i);
                    break;
                }
            }
        }
	}
	else if (!QFile(activeirs).exists())
	{
		ui->conv_dirpath->setText(AppConfig::instance().getIrsPath());
		ui->convTabs->setCurrentIndex(0);
		reloadIRS();
	}
	else
	{
        ui->convTabs->setCurrentIndex(0);

        QDir d2 = QFileInfo(activeirs).absoluteDir();
        ui->conv_dirpath->setText(d2.absolutePath());
        reloadIRS();

        if (ui->conv_files->count() >= 1)
        {
            for (int i = 0; i < ui->conv_files->count(); i++)
            {
                if (ui->conv_files->item(i)->text() == QFileInfo(activeirs).fileName())
                {
                    ui->conv_files->setCurrentRow(i);
                    break;
                }
            }
        }
	}
}

void MainWindow::reloadIRS()
{
	lockirsupdate = true;
	QDir        path(ui->conv_dirpath->text());
	QStringList nameFilter("*.irs");
	nameFilter.append("*.wav");
	nameFilter.append("*.flac");
	QStringList files = path.entryList(nameFilter);
	ui->conv_files->clear();

	if (files.empty())
	{
		QListWidgetItem *placeholder = new QListWidgetItem;
		placeholder->setText("No IRS files found");
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
		ui->conv_files->addItem(placeholder);
	}
	else
	{
		ui->conv_files->addItems(files);

        ui->conv_files->clearSelection();

        if (QFile(activeirs).exists() && !activeirs.contains(AppConfig::instance().getPath("irs_favorites")))
        {
            if (ui->conv_files->count() >= 1)
            {
                for (int i = 0; i < ui->conv_files->count(); i++)
                {
                    if (ui->conv_files->item(i)->text() == QFileInfo(activeirs).fileName())
                    {
                        ui->conv_files->setCurrentRow(i);
                        break;
                    }
                }
            }
        }
	}

	lockirsupdate = false;
}

void MainWindow::reloadIRSFav()
{
	lockirsupdate = true;
	QString     absolute = QFileInfo(AppConfig::instance().getDspConfPath()).absoluteDir().absolutePath();
	QDir        path(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites"));
	QStringList nameFilter("*.wav");
	nameFilter.append("*.irs");
	nameFilter.append("*.flac");
	QStringList files = path.entryList(nameFilter);
	ui->conv_files_fav->clear();

	if (files.empty())
	{
		QListWidgetItem *placeholder = new QListWidgetItem;
		placeholder->setText("Nothing here yet...");
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
		ui->conv_files_fav->addItem(placeholder);
		QListWidgetItem *placeholder2 = new QListWidgetItem;
		placeholder2->setText("Bookmark some IRS files in the 'filesystem' tab");
		placeholder2->setFlags(placeholder2->flags() & ~Qt::ItemIsEnabled);
		ui->conv_files_fav->addItem(placeholder2);
	}
	else
	{
		ui->conv_files_fav->addItems(files);
	}

	lockirsupdate = false;
}

void MainWindow::convolverWaveformEdit()
{
	bool    ok;
	QString text = QInputDialog::getText(this, tr("Advanced waveform editing"),
	                                     tr("Advanced waveform editing (6 semicolon-separated values; default: -80;-100;0;0;0;0)"), QLineEdit::Normal,
	                                     irAdvancedWaveformEditing, &ok,
	                                     Qt::WindowFlags(), Qt::ImhFormattedNumbersOnly);

	if (ok && !text.isEmpty())
	{
		irAdvancedWaveformEditing = text;

		applyConfig();
    }
}

EELEditor *MainWindow::eelEditor() const
{
    return m_eelEditor;
}

// ---Liveprog
void MainWindow::reloadLiveprog()
{
	lockliveprogupdate = true;
	QDir        path(ui->liveprog_dirpath->text());
	QStringList nameFilter("*.eel");
	QStringList files = path.entryList(nameFilter);
	ui->liveprog_files->clear();

	if (files.empty())
	{
		QListWidgetItem *placeholder = new QListWidgetItem;
		placeholder->setText("No EEL files found");
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
		ui->liveprog_files->addItem(placeholder);
	}
	else
	{
		ui->liveprog_files->addItems(files);
	}

	if (ui->liveprog_files->count() >= 1)
	{
		for (int i = 0; i < ui->liveprog_files->count(); i++)
		{
			if (ui->liveprog_files->item(i)->text() == QFileInfo(activeliveprog).fileName())
			{
				ui->liveprog_files->setCurrentRow(i);
				setLiveprogSelection(activeliveprog);
				break;
			}
		}
	}

	lockliveprogupdate = false;
}

void MainWindow::setLiveprogSelection(QString path)
{
	_eelparser->loadFile(path);
	ui->liveprog_name->setText(_eelparser->getDescription());

	QLayoutItem *item;

	while ((item = ui->liveprog_ui_container->layout()->takeAt(0)) != NULL )
	{
		delete item->widget();
		delete item;
	}

	EELProperties props = _eelparser->getProperties();

	for (EELBaseProperty *propbase : props)
	{
		if (propbase->getType() == EELPropertyType::NumberRange)
		{
			EELNumberRangeProperty<float> *prop        = dynamic_cast<EELNumberRangeProperty<float>*>(propbase);
			bool                           handleAsInt = is_integer(prop->getStep());
			QLabel                        *lbl         = new QLabel(this);
			QAnimatedSlider               *sld         = new QAnimatedSlider(this);
			lbl->setText(prop->getDescription());

			sld->setMinimum(prop->getMinimum() * 100);
			sld->setMaximum(prop->getMaximum() * 100);
			sld->setValue(prop->getMinimum() * 100);
			sld->setValueA(prop->getValue() * 100);
			sld->setOrientation(Qt::Horizontal);
			sld->setToolTip(QString::number(prop->getValue()));
			sld->setProperty("isCustomEELProperty", true);
			sld->setProperty("handleAsInt",         handleAsInt);

			sld->setObjectName(prop->getKey());
			sld->installEventFilter(new ScrollFilter);

			ui->liveprog_ui_container->layout()->addWidget(lbl);
			ui->liveprog_ui_container->layout()->addWidget(sld);

			connect(sld, SIGNAL(valueChangedA(int)),       this, SLOT(updateUnitLabel(int)));
			connect(sld, &QAbstractSlider::sliderReleased, [this, sld, prop] {
				float val = sld->valueA() / 100.f;
				prop->setValue(val);
				_eelparser->manipulateProperty(prop);
				ui->liveprog_reset->setEnabled(_eelparser->canLoadBackup());

                audioService->reloadLiveprog();
			});

		}
	}

	if (props.isEmpty())
	{
		QLabel *lbl = new QLabel(this);
		lbl->setText("No customizable parameters");
		ui->liveprog_ui_container->layout()->addWidget(lbl);
		ui->liveprog_reset->hide();
	}
	else
	{
		ui->liveprog_reset->show();
		ui->liveprog_reset->setEnabled(_eelparser->canLoadBackup());
	}
}

void MainWindow::resetLiveprogParams()
{
	if (!_eelparser->loadBackup())
	{
		QMessageBox::warning(this, "Error", "Cannot load backup\nThe backup file doesn't exist anymore.");
	}

    audioService->reloadLiveprog();

	setLiveprogSelection(_eelparser->getPath());
}

void MainWindow::updateFromEelEditor(QString path)
{
	if (_eelparser->getPath() == path)
	{
		_eelparser->deleteBackup();
		setLiveprogSelection(_eelparser->getPath());
	}
	else
	{
		EELParser parser;
		parser.loadFile(path);
		parser.deleteBackup();

        audioService->reloadLiveprog();
	}
}

int MainWindow::extractDefaultEelScripts(bool allowOverride,
                                         bool user)
{
	QDirIterator it(":/assets/liveprog", QDirIterator::NoIteratorFlags);
	int          i = 0;

	while (it.hasNext())
	{
		QFile   eel(it.next());
		QString name    = QFileInfo(eel).fileName();
		QString newpath = AppConfig::instance().getLiveprogPath() + "/" + name;

		if (QFile(newpath).exists() && !allowOverride)
		{
			continue;
		}

        if(!QDir(AppConfig::instance().getLiveprogPath()).exists())
        {
            QDir().mkpath(AppConfig::instance().getLiveprogPath());
        }

		QFile file(newpath);

		if (eel.open(QIODevice::ReadOnly | QIODevice::Text) &&
		    file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream stream(&file);
			stream << eel.readAll();
			file.close();
			eel.close();
		}

		i++;
	}

	if (i > 0)
	{
        Log::debug(QString("MainWindow::extractDefaultEelScripts: %1 default eel files extracted").arg(i));
	}

	if (user)
	{
		if (i > 0)
		{
			QMessageBox::information(this, "Extract scripts", QString("%1 script(s) have been restored").arg(i));
		}
		else
		{
			QMessageBox::information(this, "Extract scripts", QString("No scripts have been extracted"));
		}
	}

	return i;
}

// ---Helper
void MainWindow::setEq(const QVector<double> &data)
{
	lockapply = true;
	ui->eq_widget->setBands(QVector<double>(data));
	lockapply = false;
    onUpdate();
}

void MainWindow::resetEQ()
{
	ui->eqpreset->setCurrentIndex(0);
	ui->eq_dyn_widget->load(DEFAULT_GRAPHICEQ);
	setEq(PresetProvider::EQ::defaultPreset());
	applyConfig();
}

void MainWindow::updateEqStringFromWidget()
{
	QString currentEqPresetName =
		PresetProvider::EQ::reverseLookup(ui->eq_widget->getBands());

	ui->eqpreset->setCurrentText(currentEqPresetName);
}

void MainWindow::updateEQMode()
{
	bool isFixed = ui->eq_r_fixed->isChecked();
	setEqMode(isFixed ? 0 : 1);
}

void MainWindow::setEqMode(int mode)
{
	ui->eq_holder->setCurrentIndex(mode);
	ui->eqpreset->setEnabled(!mode);
	ui->eq_r_flex->setChecked(mode);
	ui->eq_r_fixed->setChecked(!mode);
}

// ---GraphicEQ States
void MainWindow::restoreGraphicEQView()
{
	QVariantMap state;
	state = ConfigIO::readFile(AppConfig::instance().getGraphicEQConfigFilePath());

	if (state.count() >= 1)
	{
		ui->graphicEq->loadPreferences(state);
	}
	else
	{
		ConfigContainer pref;
		pref.setValue("scrollX", 165.346);
		pref.setValue("scrollY", 50);
		pref.setValue("zoomX",   0.561);
		pref.setValue("zoomY",   0.713);
		ui->graphicEq->loadPreferences(pref.getConfigMap());
	}
}

void MainWindow::saveGraphicEQView()
{
	QVariantMap state;
	ui->graphicEq->storePreferences(state);
	ConfigIO::writeFile(AppConfig::instance().getGraphicEQConfigFilePath(),
	                    state);
}

// ---Connect UI-Signals
void MainWindow::connectActions()
{
	QString                 absolute = QFileInfo(AppConfig::instance().getDspConfPath()).absoluteDir().absolutePath();

	QList<QAnimatedSlider*> registerValueAChange({
		ui->analog_tubedrive, ui->stereowide_level, ui->bs2b_fcut, ui->bs2b_feed, ui->bass_maxgain,
		ui->limthreshold, ui->limrelease, ui->comp_maxrelease, ui->comp_maxattack, ui->comp_aggressiveness,
		ui->rev_osf, ui->rev_erf, ui->rev_era, ui->rev_erw, ui->rev_lci, ui->rev_lcb, ui->rev_lcd,
		ui->rev_lco, ui->rev_finalwet, ui->rev_finaldry, ui->rev_wet, ui->rev_width, ui->rev_spin, ui->rev_wander, ui->rev_decay,
		ui->rev_delay, ui->rev_bass, ui->postgain
	});

	QList<QWidget*> registerSliderRelease({
		ui->stereowide_level, ui->bs2b_fcut, ui->bs2b_feed, ui->rev_osf, ui->rev_erf, ui->rev_era, ui->rev_erw,
		ui->rev_lci, ui->rev_lcb, ui->rev_lcd, ui->rev_lco, ui->rev_finalwet, ui->rev_finaldry, ui->rev_wet, ui->rev_width, ui->rev_spin,
		ui->rev_wander, ui->rev_decay, ui->rev_delay, ui->rev_bass, ui->analog_tubedrive, ui->limthreshold,
		ui->limrelease, ui->comp_maxrelease, ui->comp_maxattack, ui->comp_aggressiveness,
		ui->bass_maxgain, ui->postgain
	});

	QList<QWidget*> registerClick({
		ui->bassboost, ui->bs2b, ui->stereowidener, ui->analog, ui->reverb, ui->enable_eq, ui->enable_comp, ui->ddc_enable, ui->conv_enable,
		ui->graphicEq->chk_enable, ui->liveprog_enable
	});

	foreach(QWidget * w, registerValueAChange)
	connect(w, SIGNAL(valueChangedA(int)), this, SLOT(updateUnitLabel(int)));

	foreach(QWidget * w, registerSliderRelease)
    connect(w, SIGNAL(sliderReleased()), this, SLOT(onUpdate()));

	foreach(QWidget * w, registerClick)
	connect(w,                      SIGNAL(clicked()),                this, SLOT(onUpdate()));

	connect(ui->disableFX,          SIGNAL(clicked()),                this, SLOT(disableFx()));
	connect(ui->reseteq,            SIGNAL(clicked()),                this, SLOT(resetEQ()));
	connect(ui->cpreset,            SIGNAL(clicked()),                this, SLOT(dialogHandler()));
	connect(ui->set,                SIGNAL(clicked()),                this, SLOT(dialogHandler()));

	connect(ui->eq_r_fixed,         SIGNAL(clicked()),                this, SLOT(applyConfig()));
	connect(ui->eq_r_flex,          SIGNAL(clicked()),                this, SLOT(applyConfig()));

    connect(ui->eqfiltertype,       SIGNAL(currentIndexChanged(int)), this, SLOT(onUpdate()));
    connect(ui->eqinterpolator,     SIGNAL(currentIndexChanged(int)), this, SLOT(onUpdate()));
    connect(ui->conv_ir_opt,        SIGNAL(currentIndexChanged(int)), this, SLOT(onUpdate()));

	connect(ui->eq_widget,          SIGNAL(bandsUpdated()),           this, SLOT(applyConfig()));
	connect(ui->eq_widget,          SIGNAL(mouseReleased()),          this, SLOT(updateEqStringFromWidget()));
	connect(ui->eqpreset,           SIGNAL(currentIndexChanged(int)), this, SLOT(eqPresetSelectionUpdated()));
	connect(ui->roompresets,        SIGNAL(currentIndexChanged(int)), this, SLOT(reverbPresetSelectionUpdated()));

	connect(ui->conv_adv_wave_edit, SIGNAL(clicked()),                this, SLOT(convolverWaveformEdit()));

	connect(ui->crossfeed_mode,     SIGNAL(currentIndexChanged(int)), this, SLOT(bs2bPresetSelectionUpdated()));

    connect(ui->graphicEq,          &GraphicEQFilterGUI::mouseUp,     this, &MainWindow::onUpdate);
    connect(ui->eq_dyn_widget,      &GraphicEQFilterGUI::mouseUp,     this, &MainWindow::onUpdate);
	connect(ui->graphicEq,          &GraphicEQFilterGUI::updateModel, this, [this](bool isMoving)
	{
		if (!isMoving)
		{
            onUpdate();
		}
	});
	connect(ui->eq_dyn_widget, &GraphicEQFilterGUI::updateModel, this, [this](bool isMoving)
	{
		if (!isMoving)
		{
            onUpdate();
		}
	});

	connect(ui->ddc_reload, SIGNAL(clicked()),                  this, SLOT(reloadDDC()));
	connect(ui->ddc_files,  &QListWidget::itemSelectionChanged, [this] {
		if (lockddcupdate || ui->ddc_files->selectedItems().count() < 1)
		{
		    return; // Clearing Selection by code != User Interaction
		}

		QString path = QDir(ui->ddc_dirpath->text()).filePath(ui->ddc_files->selectedItems().first()->text());

		if (QFileInfo::exists(path) && QFileInfo(path).isFile())
		{
		    activeddc = path;
		}

		onUpdate();
	});
	connect(ui->ddc_select, &QPushButton::clicked, [this] {
		QFileDialog *fd = new QFileDialog;
		fd->setFileMode(QFileDialog::Directory);
		fd->setOption(QFileDialog::ShowDirsOnly);
		fd->setViewMode(QFileDialog::Detail);
		QString result = fd->getExistingDirectory();

		if (result != "")
		{
		    ui->ddc_dirpath->setText(result);
		    reloadDDC();
		}
	});
	connect(ui->ddcTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &, const QItemSelection &) {
		QItemSelectionModel *select = ui->ddcTable->selectionModel();
		QString ddc_coeffs;

		if (select->hasSelection())
		{
		    lockddcupdate    = true;
		    ui->ddc_files->clearSelection();
		    lockddcupdate    = false;
		    int index        = select->selectedRows().first().row();
		    ddc_coeffs      += "SR_44100:";
		    ddc_coeffs      += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index, 2)).toString();
		    ddc_coeffs      += "\nSR_48000:";
		    ddc_coeffs      += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index, 3)).toString();
		    QString absolute = QFileInfo(AppConfig::instance().getDspConfPath()).absoluteDir().absolutePath();
		    QFile file(absolute + "/temp.vdc");

		    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		    {
		        file.write(ddc_coeffs.toUtf8().constData());
			}

		    file.close();

		    auto newId = ui->ddcTable->model()->data(ui->ddcTable->model()->index(index, 4)).toString();

		    if (newId != "0")
		    {
                AppConfig::instance().set(AppConfig::VdcLastDatabaseId, newId);
			}

            /* Workaround: Load different file to make sure the config parser notices a change and reloads the DDC engine */
            DspConfig::instance().set(DspConfig::ddc_file, "");
            DspConfig::instance().commit();

		    activeddc = absolute + "/temp.vdc";
		}

		onUpdate();
	});

	connect(ui->liveprog_reload, SIGNAL(clicked()),                  this, SLOT(reloadLiveprog()));
	connect(ui->liveprog_files,  &QListWidget::itemSelectionChanged, [this] {
		if (lockliveprogupdate || ui->liveprog_files->selectedItems().count() < 1)
		{
		    return; // Clearing Selection by code != User Interaction
		}

		QString path = QDir(ui->liveprog_dirpath->text()).filePath(ui->liveprog_files->selectedItems().first()->text());

		if (QFileInfo::exists(path) && QFileInfo(path).isFile())
		{
		    activeliveprog = path;
		}

		setLiveprogSelection(activeliveprog);

		onUpdate();
	});
	connect(ui->liveprog_select, &QPushButton::clicked, [this] {
		QFileDialog *fd = new QFileDialog;
		fd->setFileMode(QFileDialog::Directory);
		fd->setOption(QFileDialog::ShowDirsOnly);
		fd->setViewMode(QFileDialog::Detail);
		QString result = fd->getExistingDirectory();

		if (result != "")
		{
		    ui->liveprog_dirpath->setText(result);
		    reloadLiveprog();
		}
	});

    connect(ui->conv_reload, &QAbstractButton::clicked, this, [this] {
        reloadIRS();
    });
    connect(ui->conv_files, &QListWidget::itemSelectionChanged, [this] {
		if (lockirsupdate || ui->conv_files->selectedItems().count() < 1)
		{
		    return; // Clearing Selection by code != User Interaction
		}

		QString path = QDir(ui->conv_dirpath->text()).filePath(ui->conv_files->selectedItems().first()->text());

		if (QFileInfo::exists(path) && QFileInfo(path).isFile())
		{
		    activeirs = path;
		}

		onUpdate();
	});
	connect(ui->conv_select, &QPushButton::clicked, [this] {
		QFileDialog *fd = new QFileDialog;
		fd->setFileMode(QFileDialog::Directory);
		fd->setOption(QFileDialog::ShowDirsOnly);
		fd->setViewMode(QFileDialog::Detail);
		QString result = fd->getExistingDirectory();

		if (result != "")
		{
		    ui->conv_dirpath->setText(result);
		    reloadIRS();
		}
	});
	connect(ui->conv_bookmark, &QPushButton::clicked, [this, absolute] {
		if (ui->conv_files->selectedItems().count() < 1)
		{
		    return; // Clearing Selection by code != User Interaction

		}

		const QString src   = QDir(ui->conv_dirpath->text()).filePath(ui->conv_files->selectedItems().first()->text());
		const QString &dest = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files->selectedItems().first()->text());

		if (QFile::exists(dest))
		{
		    QFile::remove(dest);
		}

		QFile::copy(src, dest);
        Log::debug("MainWindow::ConvBookmard: Adding " + src + " to bookmarks");
		reloadIRSFav();
	});
	connect(ui->conv_files_fav, &QListWidget::itemSelectionChanged, [this, absolute] {
		if (lockirsupdate || ui->conv_files_fav->selectedItems().count() < 1)
		{
		    return; // Clearing Selection by code != User Interaction
		}

		QString path = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());

		if (QFileInfo::exists(path) && QFileInfo(path).isFile())
		{
		    activeirs = path;
		}

		onUpdate();
	});
	connect(ui->conv_fav_rename, &QPushButton::clicked, [this, absolute] {
		if (ui->conv_files_fav->selectedItems().count() < 1)
		{
		    return;
		}

		bool ok;
		QString text     = QInputDialog::getText(this, "Rename",
		                                         "New Name", QLineEdit::Normal,
		                                         ui->conv_files_fav->selectedItems().first()->text(), &ok);
		QString fullpath = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());;
		QString dest     = QDir::cleanPath(absolute + QDir::separator() + "irs_favorites");

		if (ok && !text.isEmpty())
		{
		    QFile::rename(fullpath, QDir(dest).filePath(text));
		}

		reloadIRSFav();
	});
	connect(ui->conv_fav_remove, &QPushButton::clicked, [this, absolute] {
		if (ui->conv_files_fav->selectedItems().count() < 1)
		{
		    return;
		}

		QString fullpath = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());;

		if (!QFile::exists(fullpath))
		{
		    QMessageBox::warning(this, "Error", "Selected File doesn't exist", QMessageBox::Ok);
		    reloadIRSFav();
		    return;
		}

		QFile file(fullpath);
		file.remove();
        Log::debug("MainWindow::ConvFavRemove: Removed " + fullpath + " from favorites");
		reloadIRSFav();
	});

	connect(ui->graphicEq, &GraphicEQFilterGUI::autoeqClicked, [this] {
		AutoEQSelector *sel = new AutoEQSelector(this);
		sel->setModal(true);

		if (sel->exec() == QDialog::Accepted)
		{
		    HeadphoneMeasurement hp = sel->getSelection();

		    if (hp.getGraphicEQ() == "")
		    {
		        QMessageBox::warning(this, "Error", "Empty equalizer data.\n\nEither your network connection is experiencing issues, or you are being rate-limited by GitHub.\nKeep in mind that you can only send 60 web requests per hour to this API.\n\nYou can check your current rate limit status here: https://api.github.com/rate_limit");
			}
		    else
		    {
		        ui->graphicEq->load(hp.getGraphicEQ());
		        onUpdate();
			}
		}

		sel->deleteLater();
	});

	connect(ui->eq_r_fixed,          &QRadioButton::clicked,    this, &MainWindow::updateEQMode);
	connect(ui->eq_r_flex,           &QRadioButton::clicked,    this, &MainWindow::updateEQMode);

	connect(m_eelEditor,             &EELEditor::scriptSaved,   this, &MainWindow::updateFromEelEditor);
	connect(ui->liveprog_reset,      &QAbstractButton::clicked, this, &MainWindow::resetLiveprogParams);
	connect(ui->liveprog_editscript, &QAbstractButton::clicked, [this] {
		if (activeliveprog.isEmpty())
		{
		    QMessageBox::warning(this, "Error", "No EEL file loaded.\n"
		                         "Please select one in the list on the left side.");
		    return;
		}
		else if (!QFile(activeliveprog).exists())
		{
		    QMessageBox::warning(this, "Error", "Selected EEL file does not exist anymore.\n"
		                         "Please select another one");
		    return;
		}

		m_eelEditor->show();
        m_eelEditor->raise();
		m_eelEditor->openNewScript(activeliveprog);
	});

    connect(ui->ddctoolbox_install, &QAbstractButton::clicked, []{
        system("sh -c \"xdg-open https://github.com/thepbone/DDCToolbox\""); // QDesktopServices::openUrl is broken on some KDE systems
    });
}

void MainWindow::launchFirstRunSetup()
{
	QHBoxLayout            *lbLayout = new QHBoxLayout;
	QMessageOverlay        *lightBox = new QMessageOverlay(this);
    FirstLaunchWizard      *wiz      = new FirstLaunchWizard(audioService, lightBox);
	QGraphicsOpacityEffect *eff      = new QGraphicsOpacityEffect(lightBox);
	QPropertyAnimation     *a        = new QPropertyAnimation(eff, "opacity");

	lightBox->setGraphicsEffect(eff);
	lightBox->setLayout(lbLayout);
	lightBox->layout()->addWidget(wiz);
	lightBox->show();

	a->setDuration(500);
	a->setStartValue(0);
	a->setEndValue(1);
	a->setEasingCurve(QEasingCurve::InBack);
	a->start(QPropertyAnimation::DeleteWhenStopped);

	connect(wiz, &FirstLaunchWizard::wizardFinished, [ = ]
	{
		QPropertyAnimation *b = new QPropertyAnimation(eff, "opacity");
		b->setDuration(500);
		b->setStartValue(1);
		b->setEndValue(0);
		b->setEasingCurve(QEasingCurve::OutCirc);
		b->start(QPropertyAnimation::DeleteWhenStopped);

		connect(b, &QAbstractAnimation::finished, [ = ]()
		{
            AppConfig::instance().set(AppConfig::SetupDone, true);
			lightBox->hide();
			settings_dlg->refreshAll();
			lightBox->deleteLater();
		});
	});
}
