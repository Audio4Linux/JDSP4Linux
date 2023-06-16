#ifdef USE_PULSEAUDIO
#include <PulseAudioService.h>
#else
#include <PipewireAudioService.h>
#endif

#include "IAudioService.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "config/AppConfig.h"
#include "config/ConfigContainer.h"
#include "config/ConfigIO.h"
#include "config/DspConfig.h"
#include "data/AssetManager.h"
#include "data/EelParser.h"
#include "data/model/VdcDatabaseModel.h"
#include "data/PresetManager.h"
#include "data/VersionContainer.h"
#include "interface/event/EventFilter.h"
#include "interface/event/ScrollFilter.h"
#include "interface/fragment/FirstLaunchWizard.h"
#include "interface/fragment/PresetFragment.h"
#include "interface/fragment/SettingsFragment.h"
#include "interface/fragment/StatusFragment.h"
#include "interface/QMessageOverlay.h"
#include "interface/TrayIcon.h"
#include "utils/AutoStartManager.h"
#include "utils/Common.h"
#include "utils/dbus/ClientProxy.h"
#include "utils/dbus/IpcHandler.h"
#include "utils/dbus/ServerAdaptor.h"
#include "utils/DebuggerUtils.h"
#include "utils/Log.h"
#include "utils/OverlayMsgProxy.h"
#include "utils/SingleInstanceMonitor.h"
#include "utils/StyleHelper.h"

//#include <audiostreamengine.h>
#include <Animation/Animation.h>
#include <eeleditor.h>
#include <AeqSelector.h>
#include <LiquidEqualizerWidget.h>
#include <utils/DesktopServices.h>
//#include <spectrograph.h>

#include <QButtonGroup>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSessionManager>
#include <QWhatsThis>

using namespace std;

MainWindow::MainWindow(bool     statupInTray,
                       QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Prepare audio subsystem
    {
        Log::information("============ Initializing audio service ============");
#ifdef USE_PULSEAUDIO
        Log::information("Compiled with PulseAudio support.");
        Log::information("This application flavor does not support PipeWire or its PulseAudio compatibility mode.");
        Log::information("If you want to use this application with PipeWire, you need to recompile this app with proper support enabled.");
        Log::information("Refer to the README for more detailed information.");
        Log::information("");
        _audioService = new PulseAudioService();
#else
        Log::information("Compiled with PipeWire support.");
        Log::information("This application flavor does not support PulseAudio.");
        Log::information("If you want to use this application with PulseAudio, you need to recompile this app with proper support enabled.");
        Log::information("Refer to the README for more detailed information.");
        Log::information("");
        Log::debug("Blocklisted apps: " + AppConfig::instance().get<QString>(AppConfig::AudioAppBlocklist) /* explicitly use as QString here */);
        Log::debug("Blocklist mode: " + QString((AppConfig::instance().get<bool>(AppConfig::AudioAppBlocklistInvert) ? "allow" : "block")));
        _audioService = new PipewireAudioService();
#endif
        connect(&DspConfig::instance(), &DspConfig::updated, _audioService, &IAudioService::update);
        connect(&DspConfig::instance(), &DspConfig::updatedExternally, _audioService, &IAudioService::update);
    }

    // Allocate resources
    {
        _startupInTraySwitch = statupInTray;

        _styleHelper         = new StyleHelper(this);
        _eelEditor           = new EELEditor(this);
        _trayIcon            = new TrayIcon(this);
        _ipcHandler          = new IpcHandler(_audioService, this);
        _autostart           = new AutostartManager(this);

        _appMgrFragment = new FragmentHost<AppManagerFragment*>(new AppManagerFragment(_audioService->appManager(), this), WAF::BottomSide, this);
        _statusFragment = new FragmentHost<StatusFragment*>(new StatusFragment(this), WAF::BottomSide, this);
        _presetFragment = new FragmentHost<PresetFragment*>(new PresetFragment(_audioService, this), WAF::LeftSide, this);
        _settingsFragment = new FragmentHost<SettingsFragment*>(new SettingsFragment(_trayIcon, _audioService, _autostart, this), WAF::BottomSide, this);
    }

    // Prepare base UI
    {
        Log::information("============ Initializing user interface ============");

        this->setWindowIcon(QIcon::fromTheme("jamesdsp", QIcon(":/icons/icon.png")));

        // Restore window size and position
        const QByteArray geometry = AppConfig::instance().get<QByteArray>(AppConfig::LastWindowGeometry);
        if (!geometry.isEmpty())
        {
            restoreGeometry(geometry);
        }

        // Equalizer
        QButtonGroup eq_mode;
        eq_mode.addButton(ui->eq_r_fixed);
        eq_mode.addButton(ui->eq_r_flex);
        ui->eq_widget->setBands(PresetProvider::EQ::defaultPreset(), false);
        ui->eq_dyn_widget->setSidebarHidden(true);
        ui->eq_dyn_widget->set15BandFreeMode(true);

        ConfigContainer pref;
        pref.setValue("scrollX", 160);
        pref.setValue("scrollY", 311);
        pref.setValue("zoomX",   0.561);
        pref.setValue("zoomY",   1.651);
        ui->eq_dyn_widget->loadPreferences(pref.getConfigMap());

        // GraphicEQ
        ui->graphicEq->setEnableSwitchVisible(true);
        ui->graphicEq->setAutoEqAvailable(true);
    }

    // Allocate pointers and init important variables
    {
        connect(&PresetManager::instance(), &PresetManager::wantsToWriteConfig, this, &MainWindow::applyConfig);
        connect(&PresetManager::instance(), &PresetManager::presetAutoloaded, this, [this](const QString& device){
            ui->info->setAnimatedText(tr("%1 connected - Preset loaded automatically").arg(device), true);
        });

        connect(_audioService, &IAudioService::outputDeviceChanged, &PresetManager::instance(), &PresetManager::onOutputDeviceChanged);

        // Convolver file info
        ConvolverInfoEventArgs ciArgs;
        ciArgs.channels = ciArgs.frames = -1;
        onConvolverInfoChanged(ciArgs);
        connect(_audioService, &IAudioService::convolverInfoChanged, this, &MainWindow::onConvolverInfoChanged);

       _eelEditor->attachHost(_audioService);
       connect(_eelEditor, &EELEditor::executionRequested, [this](QString path){
            if (QFileInfo::exists(path) && QFileInfo(path).isFile())
            {
                ui->liveprog->setCurrentLiveprog(path);
            }
            else
            {
                QMessageBox::critical(_eelEditor, tr("Cannot execute script"),
                                      tr("The current EEL file (at '%1') does not exist anymore on the filesystem. Please reopen the file manually.").arg(path));
                return;
            }

            applyConfig();

            if(path == ui->liveprog->currentLiveprog())
            {
                _audioService->reloadLiveprog();
            }
        });
    }

    // Prepare tray icon
    {
        connect(_trayIcon, &TrayIcon::iconActivated, this, &MainWindow::onTrayIconActivated);
        connect(_trayIcon, &TrayIcon::loadReverbPreset, this, [this](const QString &preset)
        {
            if(preset == "off"){
                ui->reverb->setChecked(false);
                applyConfig();
                return;
            }

            ui->reverb->setChecked(true);
            ui->roompresets->setCurrentText(preset);
            onReverbPresetUpdated();
        });
        connect(_trayIcon, &TrayIcon::restart, this, &MainWindow::onRelinkRequested);
        connect(_trayIcon, &TrayIcon::loadEqPreset, this, [this](const QString &preset)
        {
            ui->enable_eq->setChecked(true);

            if (preset == PresetProvider::EQ::defaultPresetName())
            {
                resetEQ();
            }
            else
            {
                ui->eqpreset->setCurrentText(preset);
                onEqPresetUpdated();
            }
        });
        connect(_trayIcon, &TrayIcon::loadCrossfeedPreset, this, [this](int preset)
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
            onBs2bPresetUpdated();
        });
        connect(_trayIcon, &TrayIcon::loadIrs, this, [this](const QString &irs)
        {
            _currentImpulseResponse = irs;
            ui->conv_enable->setChecked(true);
            determineIrsSelection();
            applyConfig();
        });
        connect(_trayIcon, &TrayIcon::loadPreset, &PresetManager::instance(), &PresetManager::loadFromPath);
        connect(_trayIcon, &TrayIcon::changeDisableFx, ui->disableFX, &QPushButton::setChecked);
        connect(_trayIcon, &TrayIcon::changeDisableFx, this,          &MainWindow::applyConfig);

        _trayIcon->setTrayVisible(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled) || _startupInTraySwitch);
    }

    // Populate preset lists
    {
        for (const auto &preset : PresetProvider::EQ::EQ_LOOKUP_TABLE().keys())
        {
            ui->eqpreset->addItem(preset);
        }

        for (const auto &preset : PresetProvider::Reverb::getPresetNames())
        {
            ui->roompresets->addItem(preset);
        }

        for (const auto &[key, value] : PresetProvider::BS2B::BS2B_LOOKUP_TABLE().toStdMap())
        {
            ui->crossfeed_mode->addItem(key, value);
        }
    }

    // Load config and connect fragment signals
    {
        connect(&DspConfig::instance(), &DspConfig::configBuffered, this, &MainWindow::loadConfig);
        DspConfig::instance().load();

        connect(_settingsFragment->fragment(), &SettingsFragment::launchSetupWizard,       this, &MainWindow::launchFirstRunSetup);
        connect(_settingsFragment->fragment(), &SettingsFragment::reopenSettings, _settingsFragment, &FragmentHost<SettingsFragment*>::slideOutIn);
        connect(_styleHelper, &StyleHelper::iconColorChanged, _settingsFragment->fragment(), &SettingsFragment::updateButtonStyle);
    }

    // Init 3-dot menu button
    {
        // Fix tool button height
        int tbSize = ui->disableFX->height() - 4;
        ui->set->setMinimumSize(tbSize + 3, tbSize);
        ui->toolButton->setMinimumSize(tbSize + 3, tbSize);
        ui->cpreset->setMinimumSize(tbSize + 3, tbSize);

        ui->set->setIconSize(QSize(16,16));
        ui->cpreset->setIconSize(QSize(16,16));
        ui->toolButton->setIconSize(QSize(16,16));
        ui->disableFX->setIconSize(QSize(16,16));

        // Attach menu
        QMenu *menu = new QMenu();
        menu->addAction(tr("Apps"), _appMgrFragment, &FragmentHost<AppManagerFragment*>::slideIn);
        menu->addAction(tr("Driver status"), _statusFragment, [this](){
            _statusFragment->fragment()->updateStatus(_audioService->status());
            _statusFragment->slideIn();
        });
        menu->addAction(tr("Relink audio pipeline"), this, SLOT(onRelinkRequested()));
        menu->addSeparator();
        menu->addAction(tr("Reset to defaults"), this, SLOT(onResetRequested()));
        menu->addAction(tr("Load from file"), this, SLOT(loadExternalFile()));
        menu->addAction(tr("Save to file"), this, SLOT(saveExternalFile()));
        menu->addSeparator();
        menu->addAction(tr("Open LiveprogIDE"), _eelEditor, &EELEditor::show);
        menu->addSeparator();
        menu->addAction(tr("What's this... (Select UI element)"), this, []()
        {
            QWhatsThis::enterWhatsThisMode();
        });
        ui->toolButton->setMenu(menu);

        connect(_styleHelper, &StyleHelper::iconColorChanged, this, [this](bool white){
            if (white)
            {
                ui->set->setIcon(QIcon(":/icons/settings-white.svg"));
                ui->cpreset->setIcon(QIcon(":/icons/queue-white.svg"));
                ui->toolButton->setIcon(QIcon(":/icons/menu-white.svg"));
                ui->disableFX->setIcon(QIcon(":/icons/power-white.svg"));
            }
            else
            {
                ui->set->setIcon(QIcon(":/icons/settings.svg"));
                ui->cpreset->setIcon(QIcon(":/icons/queue.svg"));
                ui->toolButton->setIcon(QIcon(":/icons/menu.svg"));
                ui->disableFX->setIcon(QIcon(":/icons/power.svg"));
            }
        });
    }

    // Prepare styles
    {
        _styleHelper->SetStyle();
        ui->eq_widget->setAccentColor(palette().highlight().color());
    }

    // Extract default EEL files if missing
    {
        if (AppConfig::instance().get<bool>(AppConfig::LiveprogAutoExtract))
        {
            AssetManager::instance().extractAll();

        }
    }

    // Setup file selectors
    {
        // DDC
        ui->ddc_files->setCurrentDirectory(AppConfig::instance().getVdcPath());
        ui->ddc_files->setFileTypes(QStringList("*.vdc"));
        connect(ui->ddc_files,  &FileSelectionWidget::fileChanged, this, &MainWindow::setVdcFile);

        ui->ddcTable->setModel(new VdcDatabaseModel(ui->ddcTable));
        ui->ddcTable->setColumnHidden(2, true);
        ui->ddcTable->setColumnHidden(3, true);
        ui->ddcTable->setColumnHidden(4, true);
        ui->ddcTable->resizeColumnsToContents();
        determineVdcSelection();

        // Convolver
        determineIrsSelection();
        ui->conv_files->setCurrentDirectory(AppConfig::instance().getIrsPath());
        ui->conv_files->setFileTypes(QStringList(std::initializer_list<QString>({"*.irs", "*.wav", "*.flac"})));
        ui->conv_files->setBookmarkDirectory(QDir(AppConfig::instance().getPath("irs_favorites")));

        ui->conv_fav->setCurrentDirectory(AppConfig::instance().getPath("irs_favorites"));
        ui->conv_fav->setFileTypes(QStringList(std::initializer_list<QString>({"*.irs", "*.wav", "*.flac"})));
        ui->conv_fav->setFileActionsVisible(true);
        ui->conv_fav->setNavigationBarVisible(false);

        connect(ui->conv_files, &FileSelectionWidget::fileChanged, this, &MainWindow::setIrsFile);
        connect(ui->conv_fav,   &FileSelectionWidget::fileChanged, this, &MainWindow::setIrsFile);
        connect(ui->conv_files, &FileSelectionWidget::fileChanged, ui->conv_fav, &FileSelectionWidget::clearCurrentFile);
        connect(ui->conv_fav,   &FileSelectionWidget::fileChanged, ui->conv_files, &FileSelectionWidget::clearCurrentFile);
        connect(ui->conv_files, &FileSelectionWidget::bookmarkAdded, ui->conv_fav, &FileSelectionWidget::enumerateFiles);

        // Liveprog
        ui->liveprog->coupleIDE(_eelEditor);
        ui->liveprog->updateList();
        connect(ui->liveprog, &LiveprogSelectionWidget::liveprogReloadRequested, _audioService, &IAudioService::reloadLiveprog);
        connect(ui->liveprog, &LiveprogSelectionWidget::unitLabelUpdateRequested, ui->info, qOverload<const QString&>(&FadingLabel::setAnimatedText));
    }

    // Connect remaining signals
    {
        connectActions();

        connect(&AppConfig::instance(), &AppConfig::themeChanged, this, [this]()
        {
            _styleHelper->SetStyle();
            ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
            ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
            ui->tabbar->redrawTabBar();
            ui->eq_widget->setAccentColor(palette().highlight().color());
        });

        connect(&AppConfig::instance(), &AppConfig::updated, this, &MainWindow::onAppConfigUpdated);
    }

    // Tabs and other UI related things
    {
        restoreGraphicEQView();
        ui->eq_widget->setAlwaysDrawHandles(AppConfig::instance().get<bool>(AppConfig::EqualizerShowHandles));

        ui->tabbar->setAnimatePageChange(true);
        ui->tabbar->setCustomStackWidget(ui->tabhost);
        ui->tabbar->setDetachCustomStackedWidget(true);
        ui->tabbar->addPage(tr("Bass/Misc"));
        ui->tabbar->addPage(tr("Sound Positioning"));
        ui->tabbar->addPage(tr("Reverb"));
        ui->tabbar->addPage(tr("Equalizer"));
        ui->tabbar->addPage(tr("Convolver"));
        ui->tabbar->addPage(tr("DDC"));
        ui->tabbar->addPage(tr("Liveprog"));
        ui->tabbar->addPage(tr("Graphic EQ"));
        ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
        ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7,QWidget#tabHostPage8,QWidget#tabHostPage9{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
        ui->tabbar->redrawTabBar();

        installUnitData();

        if (debuggerIsAttached() || system("which ddctoolbox > /dev/null 2>&1")) { // Workaround: do not call system() when GDB is attached
            connect(ui->ddctoolbox_install, &QAbstractButton::clicked, this, [this]{
                DesktopServices::openUrl("https://github.com/thepbone/DDCToolbox", this);
            });
        } else {
            ui->ddctoolbox_install->setText(tr("Launch application"));
            connect(ui->ddctoolbox_install, &QAbstractButton::clicked, this, []{
                QProcess::startDetached("ddctoolbox", QStringList());
            });
        }
    }

    // Handle first launch
    {
        if (!AppConfig::instance().get<bool>(AppConfig::SetupDone))
        {
            launchFirstRunSetup();
        }
    }

    Log::information("UI initialized");
}

MainWindow::~MainWindow()
{
    if(_audioService != nullptr)
        delete _audioService;
    delete _ipcHandler;
    delete ui;
}

// Overrides
void MainWindow::showEvent(QShowEvent *event)
{
    if(_firstShowEvent) {
        _firstShowEvent = false;
        // Deferred auto-start setup re-check
        _autostart->setup();
    }
    QMainWindow::showEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    AppConfig::instance().setBytes(AppConfig::LastWindowGeometry, saveGeometry());
    saveGraphicEQView();

#ifdef Q_OS_OSX

    if (!event->spontaneous() || !isVisible())
    {
        return;
    }

#endif

    if (_trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
    else
    {
        event->accept();
        QMainWindow::closeEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    AppConfig::instance().setBytes(AppConfig::LastWindowGeometry, saveGeometry());
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

void MainWindow::onTrayIconActivated()
{
    setVisible(!this->isVisible());

    if (isVisible())
    {
        raiseWindow();
    }

    // Hide tray icon if disabled and MainWin is visible (for cmdline force switch)
    if (!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled) && this->isVisible())
    {
        _trayIcon->setTrayVisible(false);
    }
}

void MainWindow::onConvolverInfoChanged(const ConvolverInfoEventArgs& args)
{
    ui->ir_details_full->setVisible(args.channels != -1);
    ui->ir_details_empty->setVisible(args.channels == -1);

    ui->ir_details_channel->setText(QString::number(args.channels));
    ui->ir_details_frames->setText(QString::number(args.frames));
}

void MainWindow::onAppConfigUpdated(const AppConfig::Key &key, const QVariant &value)
{
    switch(key)
    {
        case AppConfig::EqualizerShowHandles:
            ui->eq_widget->setAlwaysDrawHandles(value.toBool());
            break;
        case AppConfig::TrayIconEnabled:
            _trayIcon->setTrayVisible(value.toBool());
            break;
        default:
            break;
    }
}

// Fragment handler
void MainWindow::onFragmentRequested()
{
    if (sender() == ui->set)
    {
        _settingsFragment->fragment()->setMaximumHeight(this->height() - 20);
        _settingsFragment->slideIn();
    }
    else if (sender() == ui->cpreset)
    {
        _presetFragment->slideIn();
    }
}

void MainWindow::onResetRequested()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Reset Configuration"), tr("Are you sure?"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        DspConfig::instance().loadDefault();
    }
}

void MainWindow::onPassthroughToggled()
{
    _trayIcon->changedDisableFx(ui->disableFX->isChecked());
    applyConfig();
}

void MainWindow::onRelinkRequested()
{
    _audioService->reloadService();
    DspConfig::instance().commit();
}

// User preset management
void MainWindow::loadExternalFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Load custom audio.conf"), "", "JamesDSP Linux configuration (*.conf)");

    if (filename == "")
    {
        return;
    }

    PresetManager::instance().loadFromPath(filename);
}

void MainWindow::saveExternalFile()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save current audio.conf"), "", "JamesDSP Linux configuration (*.conf)");

    if (filename == "")
    {
        return;
    }

    if (QFileInfo(filename).suffix() != "conf")
    {
        filename.append(".conf");
    }

    applyConfig();
    PresetManager::instance().saveToPath(filename);
}

// Load/save
void MainWindow::loadConfig()
{
    _blockApply = true;

    auto dsp = &DspConfig::instance();
    dsp->get<bool>(DspConfig::master_enable);

    _trayIcon->changedDisableFx(!DspConfig::instance().get<bool>(DspConfig::master_enable));
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
    int bs2bIndex = ui->crossfeed_mode->findData(bs2bMode);
    if(bs2bIndex < 0)
        Log::error(QString("BS2B index for value %1 not found").arg(bs2bMode));
    else
        ui->crossfeed_mode->setCurrentIndex(bs2bIndex);
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
    _currentVdc = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::ddc_file));

    ui->liveprog->setActive(DspConfig::instance().get<bool>(DspConfig::liveprog_enable));
    ui->liveprog->setCurrentLiveprog(chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::liveprog_file)));

    ui->conv_enable->setChecked(DspConfig::instance().get<bool>(DspConfig::convolver_enable));
    ui->conv_ir_opt->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::convolver_optimization_mode));
    _currentImpulseResponse = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::convolver_file));
    _currentConvWaveformEdit = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::convolver_waveform_edit));

    ui->enable_eq->setChecked(DspConfig::instance().get<bool>(DspConfig::tone_enable));
    ui->eqinterpolator->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::tone_interpolation));
    ui->eqfiltertype->setCurrentIndex(DspConfig::instance().get<int>(DspConfig::tone_filtertype));

    // Parse EQ String to QMap
    QString rawEqString = chopDoubleQuotes(DspConfig::instance().get<QString>(DspConfig::tone_eq));
    bool isOldFormat = rawEqString.split(";").count() == 15;

    if (isOldFormat)
    {
        rawEqString = "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;"
                + rawEqString;
    }

    QVector<float> rawEqData;

    for (const auto &val : rawEqString.split(";"))
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

    determineEqPresetName();
    determineIrsSelection();
    determineVdcSelection();

    _blockApply = false;
}

void MainWindow::applyConfig()
{
    if(_blockApply)
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
    DspConfig::instance().set(DspConfig::ddc_file,                   QVariant(_currentVdc));

    DspConfig::instance().set(DspConfig::liveprog_enable,            QVariant(ui->liveprog->isActive()));
    DspConfig::instance().set(DspConfig::liveprog_file,              QVariant(ui->liveprog->currentLiveprog()));

    DspConfig::instance().set(DspConfig::convolver_enable,           QVariant(ui->conv_enable->isChecked()));
    DspConfig::instance().set(DspConfig::convolver_optimization_mode,QVariant(ui->conv_ir_opt->currentIndex()));
    DspConfig::instance().set(DspConfig::convolver_file,             QVariant(_currentImpulseResponse));
    DspConfig::instance().set(DspConfig::convolver_waveform_edit,    QVariant(_currentConvWaveformEdit));

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

        DspConfig::instance().set(DspConfig::tone_eq, QVariant(rawEqString));
    }
    else
    {
        QString rawEqString;
        ui->eq_dyn_widget->storeCsv(rawEqString);
        DspConfig::instance().set(DspConfig::tone_eq, QVariant(rawEqString));
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
    DspConfig::instance().set(DspConfig::graphiceq_param,           QVariant(streq));

    DspConfig::instance().commit();
    DspConfig::instance().save();

    saveGraphicEQView();
}

// Predefined presets
void MainWindow::onEqPresetUpdated()
{
    if (_blockApply)
    {
        return;
    }

    auto preset = PresetProvider::EQ::lookupPreset(ui->eqpreset->currentText());
    if (preset.size() > 0)
    {
        setEq(preset);
    }
}

void MainWindow::onBs2bPresetUpdated()
{
    if (_blockApply)
    {
        return;
    }

    const auto index = PresetProvider::BS2B::lookupPreset(ui->crossfeed_mode->currentText());

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

    applyConfig();
}

void MainWindow::onReverbPresetUpdated()
{
    if (ui->roompresets->currentText() == "..." || _blockApply)
    {
        return;
    }

    // index - 1 because 1st entry is '...'
    const auto data = PresetProvider::Reverb::lookupPreset(ui->roompresets->currentIndex() - 1);
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

    applyConfig();
}

// Status
void MainWindow::installUnitData()
{
    ui->rev_width->setProperty("unit", "%");
    ui->rev_osf->setProperty("unit", "x");

    QList<QAnimatedSlider*> div100({ui->rev_era, ui->rev_erf, ui->rev_erw, ui->rev_width, ui->rev_bass, ui->rev_spin, ui->rev_wander, ui->rev_decay,
                                    ui->analog_tubedrive});

    QList<QAnimatedSlider*> div10({ui->bs2b_feed, ui->rev_delay, ui->rev_wet, ui->rev_finalwet, ui->rev_finaldry, ui->rev_width});

    QList<QAnimatedSlider*> unitDecibel({ui->bs2b_feed, ui->rev_wet, ui->rev_finalwet, ui->rev_finaldry, ui->analog_tubedrive, ui->postgain, ui->bass_maxgain});
    QList<QAnimatedSlider*> unitMs({ui->rev_delay, ui->comp_maxattack, ui->comp_maxrelease, ui->limrelease});
    QList<QAnimatedSlider*> unitHz({ui->bs2b_fcut, ui->rev_lcb, ui->rev_lcd, ui->rev_lci, ui->rev_lco});

    foreach(auto w, div100)
        w->setProperty("divisor", 100);
    foreach(auto w, div10)
        w->setProperty("divisor", 10);

    foreach(auto w, unitDecibel)
        w->setProperty("unit", "dB");
    foreach(auto w, unitMs)
        w->setProperty("unit", "ms");
    foreach(auto w, unitHz)
        w->setProperty("unit", "Hz");
}

// DDC
void MainWindow::setVdcFile(const QString& path)
{
    _currentVdc = path;
    applyConfig();
}

void MainWindow::determineVdcSelection()
{
    ui->ddc_files->clearCurrentFile();
    ui->ddcTabs->setCurrentIndex(0);

    // File does not exist anymore
    if (!QFile(_currentVdc).exists())
    {
        ui->ddc_files->setCurrentDirectory(AppConfig::instance().getVdcPath());
    }
    // File is from database
    else if (_currentVdc == AppConfig::instance().getPath("temp.vdc"))
    {
        QString lastId = AppConfig::instance().get<QString>(AppConfig::VdcLastDatabaseId);
        ui->ddcTabs->setCurrentIndex(1);

        if (lastId.isEmpty())
        {
            return;
        }

        VdcDatabaseModel* model = static_cast<VdcDatabaseModel*>(ui->ddcTable->model());
        if (model == nullptr)
        {
            return;
        }

        QModelIndex lastIndex = model->findFirstById(lastId);
        if(lastIndex.isValid())
        {
            ui->ddcTable->selectRow(lastIndex.row());
            ui->ddcTable->scrollTo(lastIndex);
        }
    }
    // External file
    else
    {
        ui->ddc_files->setCurrentFile(_currentVdc);
    }
}

void MainWindow::onVdcDatabaseSelected(const QItemSelection &, const QItemSelection &) {
    QItemSelectionModel *select = ui->ddcTable->selectionModel();

    if (select->hasSelection())
    {
        ui->ddc_files->clearCurrentFile();

        int index        = select->selectedRows().first().row();
        auto* model      = static_cast<VdcDatabaseModel*>(ui->ddcTable->model());

        QFile file(AppConfig::instance().getPath("temp.vdc"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.write(model->composeVdcFile(index).toUtf8().constData());
        }
        file.close();

        auto newId = model->id(index);
        if (newId != "0")
        {
            AppConfig::instance().set(AppConfig::VdcLastDatabaseId, newId);
        }

        /* Workaround: Load different file to make sure the config parser notices a change and reloads the DDC engine */
        DspConfig::instance().set(DspConfig::ddc_file, "");
        DspConfig::instance().commit();

        _currentVdc = AppConfig::instance().getPath("temp.vdc");
    }

    applyConfig();
}

// IRS
void MainWindow::setIrsFile(const QString& path)
{
    _currentImpulseResponse = path;
    applyConfig();
}

void MainWindow::determineIrsSelection()
{
    ui->conv_fav->clearCurrentFile();
    ui->conv_files->clearCurrentFile();
    ui->convTabs->setCurrentIndex(0);

    // File was selected from favorites
    if (_currentImpulseResponse.contains(AppConfig::instance().getPath("irs_favorites")))
    {
        ui->conv_files->setCurrentDirectory(AppConfig::instance().getIrsPath());
        ui->conv_fav->setCurrentDirectory(AppConfig::instance().getPath("irs_favorites"));
        ui->conv_fav->setCurrentFile(_currentImpulseResponse);
        ui->convTabs->setCurrentIndex(1);
    }
    // File does not exist anymore
    else if (!QFile(_currentImpulseResponse).exists())
    {
        ui->conv_files->setCurrentDirectory(AppConfig::instance().getIrsPath());
    }
    // External file
    else
    {
        ui->conv_files->setCurrentFile(_currentImpulseResponse);
    }
}

void MainWindow::onConvolverWaveformEdit()
{
    bool    ok;
    QString text = QInputDialog::getText(this, tr("Advanced waveform editing"),
                                         tr("Advanced waveform editing (default: -80;-100;0;0;0;0)\n"
                                            "\n"
                                            "Set threshold of auto-IR-cropping and add delay to a chopped/minimum phase transformed IR.\n"
                                            "This setting is only in effect if IR optimization is enabled.\n"
                                            "\n"
                                            "1st value: Start threshold auto-cropping (dB)\n"
                                            "2nd value: End threshold auto-cropping (dB)\n"
                                            "3rd value: Channel 1 delay (samples)\n"
                                            "4th value: Channel 2 delay (samples)\n"
                                            "5th value: Channel 3 delay (samples)\n"
                                            "6th value: Channel 4 delay (samples)\n"), QLineEdit::Normal,
                                         _currentConvWaveformEdit, &ok,
                                         Qt::WindowFlags(), Qt::ImhFormattedNumbersOnly);

    if (ok && !text.isEmpty())
    {
        _currentConvWaveformEdit = text;

        applyConfig();
    }
}

// EQ
void MainWindow::setEq(const QVector<double> &data)
{
    ui->eq_widget->setBands(QVector<double>(data));
    applyConfig();
}

void MainWindow::resetEQ()
{
    ui->eqpreset->setCurrentIndex(0);
    _blockApply = true;
    ui->eq_dyn_widget->load(DEFAULT_GRAPHICEQ);
    _blockApply = false;
    setEq(PresetProvider::EQ::defaultPreset());
    // setEq calls applyConfig();
}

void MainWindow::determineEqPresetName()
{
    QString currentEqPresetName =
            PresetProvider::EQ::reverseLookup(ui->eq_widget->getBands());

    if(currentEqPresetName.isEmpty())
        ui->eqpreset->setCurrentIndex(0);
    else
        ui->eqpreset->setCurrentText(currentEqPresetName);
}

void MainWindow::onEqModeUpdated()
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

void MainWindow::onAutoEqImportRequested()
{
    auto sel = new AeqSelector(this);
    sel->setModal(true);

    if (sel->exec() == QDialog::Accepted)
    {
        auto graphicEq = sel->selection(AeqSelector::dGraphicEq);

        if (!graphicEq.isEmpty())
        {
            ui->graphicEq->load(graphicEq);
            applyConfig();
        }
    }

    sel->deleteLater();
}

// GraphicEQ states
void MainWindow::restoreGraphicEQView()
{
    QVariantMap state;
    state = ConfigIO::readFile(AppConfig::instance().getGraphicEqStatePath());

    ConfigContainer conf;
    conf.setConfigMap(state);
    if (state.count() >= 4 && conf.getInt("scrollY") != 0)
    {
        ui->graphicEq->loadPreferences(state);
    }
    else
    {
        ConfigContainer pref;
        pref.setValue("scrollX", 102);
        pref.setValue("scrollY", 825);
        pref.setValue("zoomX",   0.561);
        pref.setValue("zoomY",   3.822);
        ui->graphicEq->loadPreferences(pref.getConfigMap());
    }
}

void MainWindow::saveGraphicEQView()
{
    QVariantMap state;
    ui->graphicEq->storePreferences(state);
    ConfigIO::writeFile(AppConfig::instance().getGraphicEqStatePath(), state);
}

// Connections
void MainWindow::connectActions()
{
    QList<QAnimatedSlider*> sliders({
                              ui->stereowide_level, ui->bs2b_fcut, ui->bs2b_feed, ui->rev_osf, ui->rev_erf, ui->rev_era, ui->rev_erw,
                              ui->rev_lci, ui->rev_lcb, ui->rev_lcd, ui->rev_lco, ui->rev_finalwet, ui->rev_finaldry, ui->rev_wet, ui->rev_width, ui->rev_spin,
                              ui->rev_wander, ui->rev_decay, ui->rev_delay, ui->rev_bass, ui->analog_tubedrive, ui->limthreshold,
                              ui->limrelease, ui->comp_maxrelease, ui->comp_maxattack, ui->comp_aggressiveness,
                              ui->bass_maxgain, ui->postgain
                          });

    QList<QWidget*> registerClick({
                              ui->bassboost, ui->bs2b, ui->stereowidener, ui->analog, ui->reverb, ui->enable_eq, ui->enable_comp, ui->ddc_enable, ui->conv_enable,
                              ui->graphicEq->chk_enable
                          });

    foreach(QAnimatedSlider* w, sliders)
    {        
        connect(w, &QAnimatedSlider::stringChanged, ui->info, qOverload<const QString&>(&FadingLabel::setAnimatedText));
        connect(w, &QAnimatedSlider::valueChangedA, this, &MainWindow::applyConfig);
    }

    foreach(QWidget* w, registerClick)
        connect(w,                  SIGNAL(clicked()), this, SLOT(applyConfig()));

    connect(ui->disableFX,          &QAbstractButton::clicked, this, &MainWindow::onPassthroughToggled);
    connect(ui->reseteq,            &QAbstractButton::clicked, this, &MainWindow::resetEQ);
    connect(ui->cpreset,            &QAbstractButton::clicked, this, &MainWindow::onFragmentRequested);
    connect(ui->set,                &QAbstractButton::clicked, this, &MainWindow::onFragmentRequested);

    connect(ui->eq_r_fixed,         &QAbstractButton::clicked, this, &MainWindow::applyConfig);
    connect(ui->eq_r_flex,          &QAbstractButton::clicked, this, &MainWindow::applyConfig);

    connect(ui->eqfiltertype,       qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::applyConfig);
    connect(ui->eqinterpolator,     qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::applyConfig);
    connect(ui->conv_ir_opt,        qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::applyConfig);

    connect(ui->crossfeed_mode,     qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onBs2bPresetUpdated);
    connect(ui->eqpreset,           qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onEqPresetUpdated);
    connect(ui->roompresets,        qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onReverbPresetUpdated);

    connect(ui->eq_widget,          &LiquidEqualizerWidget::bandsUpdated, this, &MainWindow::applyConfig);
    connect(ui->eq_widget,          &LiquidEqualizerWidget::mouseReleased, this, &MainWindow::determineEqPresetName);

    connect(ui->conv_adv_wave_edit, &QAbstractButton::clicked, this, &MainWindow::onConvolverWaveformEdit);

    connect(ui->graphicEq,          &GraphicEQFilterGUI::mouseUp, this, &MainWindow::applyConfig);
    connect(ui->eq_dyn_widget,      &GraphicEQFilterGUI::mouseUp, this, &MainWindow::applyConfig);

    connect(ui->graphicEq,          &GraphicEQFilterGUI::updateModelEnd, this, &MainWindow::applyConfig);
    connect(ui->eq_dyn_widget,      &GraphicEQFilterGUI::updateModelEnd, this, &MainWindow::applyConfig);

    connect(ui->ddcTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onVdcDatabaseSelected);
    connect(ui->liveprog,           &LiveprogSelectionWidget::toggled, this, &MainWindow::applyConfig);
    connect(ui->liveprog,           &LiveprogSelectionWidget::scriptChanged, this, &MainWindow::applyConfig);

    connect(ui->graphicEq,          &GraphicEQFilterGUI::autoeqClicked, this, &MainWindow::onAutoEqImportRequested);

    connect(ui->eq_r_fixed,         &QRadioButton::clicked, this, &MainWindow::onEqModeUpdated);
    connect(ui->eq_r_flex,          &QRadioButton::clicked, this, &MainWindow::onEqModeUpdated);

    connect(_eelEditor,             &EELEditor::scriptSaved, ui->liveprog, &LiveprogSelectionWidget::updateFromEelEditor);
}

// Setup wizard
void MainWindow::launchFirstRunSetup()
{
    QHBoxLayout            *lbLayout = new QHBoxLayout;
    QMessageOverlay        *lightBox = new QMessageOverlay(this);
    FirstLaunchWizard      *wiz      = new FirstLaunchWizard(_autostart, lightBox);
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

    connect(wiz, &FirstLaunchWizard::wizardFinished, [lightBox, eff, this]
    {
        QPropertyAnimation *b = new QPropertyAnimation(eff, "opacity");
        b->setDuration(500);
        b->setStartValue(1);
        b->setEndValue(0);
        b->setEasingCurve(QEasingCurve::OutCirc);
        b->start(QPropertyAnimation::DeleteWhenStopped);

        connect(b, &QAbstractAnimation::finished, [lightBox, this]()
        {
            AppConfig::instance().set(AppConfig::SetupDone, true);
            lightBox->hide();
            _settingsFragment->fragment()->refreshAll();
            lightBox->deleteLater();
        });
    });
}

