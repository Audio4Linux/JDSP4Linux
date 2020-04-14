#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog/statusfragment.h"
#include "misc/overlaymsgproxy.h"
#include "dbus/serveradaptor.h"
#include "dbus/clientproxy.h"
#include "misc/versioncontainer.h"
#include "dialog/liquidequalizerwidget.h"
#include "misc/eventfilter.h"
#include "dialog/firstlaunchwizard.h"

#include <phantomstyle.h>
#include <Animation/Animation.h>
#include <misc/qjsontablemodel.h>
#include <dialog/autoeqselector.h>

#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QWhatsThis>
#include <QGraphicsOpacityEffect>
#include <QDebug>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <QClipboard>
#include <QInputDialog>
#include <QButtonGroup>

///TODO: --- REMOVE BEFORE RELEASE ---
#define DISABLE_DIAGNOSTICS
///TODO: -----------------------------

using namespace std;

MainWindow::MainWindow(QString exepath, bool statupInTray, bool allowMultipleInst, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    bool aboutToQuit = false;

    ui->tabhost_legacy->hide();
    ui->eq_dyn_widget->setSidebarHidden(true);
    ui->eq_dyn_widget->set15BandFreeMode(true);

    ConfigContainer pref;
    pref.setValue("scrollX",160.366);
    pref.setValue("scrollY",34.862);
    pref.setValue("zoomX",0.561);
    pref.setValue("zoomY",0.561);
    ui->eq_dyn_widget->loadPreferences(pref.getConfigMap());

    QButtonGroup eq_mode;
    eq_mode.addButton(ui->eq_r_fixed);
    eq_mode.addButton(ui->eq_r_flex);

    QDir("/tmp").mkdir("jamesdsp");

    m_exepath = exepath;
    m_startupInTraySwitch = statupInTray;

    LogHelper::clear();
    LogHelper::information("UI launched...");

    msg_notrunning = new OverlayMsgProxy(this);
    msg_launchfail = new OverlayMsgProxy(this);
    msg_versionmismatch = new OverlayMsgProxy(this);

    tray_disableAction = new QAction();
    conf = new ConfigContainer();
    m_stylehelper = new StyleHelper(this);
    m_appwrapper = new AppConfigWrapper(m_stylehelper);
    m_dbus = new DBusProxy();
    m_eelEditor = new EELEditor(this);

    m_appwrapper->loadAppConfig();

    if(!QFile(m_appwrapper->getPath()).exists()){
        QFile file(m_appwrapper->getPath());
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            stream << QString::fromStdString(default_config);
            file.close();
        }
    }

    InitializeSpectrum();

    conf->setConfigMap(readConfig());
    LoadConfig();

    preset_dlg = new PresetDlg(this);

    createTrayIcon();
    initGlobalTrayActions();

    settings_dlg = new SettingsDlg(this,this);
    log_dlg = new LogDlg(this);

    //This section checks if another instance is already running and switches to it.
    new GuiAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    bool serviceRegistrationSuccessful = connection.registerObject("/Gui", this);
    bool objectRegistrationSuccessful = connection.registerService("cf.thebone.jdsp4linux.Gui");
    if(serviceRegistrationSuccessful && objectRegistrationSuccessful)
        LogHelper::information("DBus service registration successful");
    else{
        LogHelper::warning("DBus service registration failed. Name already aquired by other instance");
        if(!allowMultipleInst){
            LogHelper::information("Attempting to switch to this instance...");
            auto m_dbInterface = new cf::thebone::jdsp4linux::Gui("cf.thebone.jdsp4linux.Gui", "/Gui",
                                                                  QDBusConnection::sessionBus(), this);
            if(!m_dbInterface->isValid())
                LogHelper::error("Critical: Unable to connect to other DBus instance. Continuing anyway...");
            else{
                QDBusPendingReply<> msg = m_dbInterface->raiseWindow();
                if(msg.isError() || msg.isValid()){
                    LogHelper::error("Critical: Other DBus instance returned (invalid) error message. Continuing anyway...");
                }
                else{
                    aboutToQuit = true;
                    LogHelper::information("Success! Waiting for event loop to exit...");
                    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
                }
            }
        }
    }

    //Cancel constructor if quitting soon
    if(aboutToQuit) return;

    //Init 3-dot menu button
    QMenu *menu = new QMenu();
    spectrum = new QAction("Reload spectrum",this);
    connect(spectrum,&QAction::triggered,this,&MainWindow::RestartSpectrum);
    menu->addAction(tr("Reload JDSP"), this,SLOT(Restart()));
    menu->addAction(spectrum);
    menu->addAction(tr("Driver status"), this,[this](){
        if(!m_dbus->isValid()){
            ShowDBusError();
            return;
        }

        StatusDialog* sd = new StatusDialog(m_dbus);
        QWidget* host = new QWidget(this);
        host->setProperty("menu", false);
        QVBoxLayout* hostLayout = new QVBoxLayout(host);
        hostLayout->addWidget(sd);
        host->hide();
        host->setAutoFillBackground(true);
        connect(sd,&StatusDialog::closePressed,this,[host](){
            WAF::Animation::sideSlideOut(host, WAF::BottomSide);
        });
        WAF::Animation::sideSlideIn(host, WAF::BottomSide);
    });
    menu->addAction(tr("Load from file"), this,SLOT(LoadExternalFile()));
    menu->addAction(tr("Save to file"), this,SLOT(SaveExternalFile()));
    menu->addAction(tr("View logs"), this,SLOT(OpenLog()));
    menu->addAction(tr("What's this..."), this,[](){QWhatsThis::enterWhatsThisMode();});
    ui->toolButton->setMenu(menu);

    //Prepare styles
    m_stylehelper->SetStyle();
    ui->eq_widget->setAccentColor(palette().highlight().color());

    //Reload DDC selection
    reloadDDCDB();
    QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
    if(!QFile(activeddc).exists()||activeddc==(absolute+"/dbcopy.vdc")){
        ui->ddc_dirpath->setText(m_appwrapper->getDDCPath());
        reloadDDC();
        if(activeddc==(absolute+"/dbcopy.vdc"))
            ui->ddcTabs->setCurrentIndex(1);
    }else{
        try {
            QDir d2 = QFileInfo(activeddc).absoluteDir();
            ui->ddc_dirpath->setText(d2.absolutePath());
            reloadDDC();
        } catch (const exception& e) {
            LogHelper::error("Failed to load previous DDC path: " + QString::fromStdString(e.what()));
            ui->ddc_dirpath->setText(m_appwrapper->getDDCPath());
            reloadDDC();
        }
    }

    //Reload IRS lists
    reloadIRSFav();
    if(!QFile(activeirs).exists()){
        ui->conv_dirpath->setText(m_appwrapper->getIrsPath());
        reloadIRS();
    }
    else if(activeirs.contains(absolute+"/irs_favorites")){
        ui->conv_dirpath->setText(m_appwrapper->getIrsPath());
        reloadIRS();
        ui->convTabs->setCurrentIndex(1);
        try {
            if(ui->conv_files_fav->count() >= 1){
                for(int i=0;i<ui->conv_files_fav->count();i++){
                    if(ui->conv_files_fav->item(i)->text()==QFileInfo(activeirs).fileName()){
                        ui->conv_files_fav->setCurrentRow(i);
                        break;
                    }
                }
            }
        } catch (const exception& e) {
            LogHelper::error("Failed to load previous fav-IRS path: " + QString::fromStdString(e.what()));
        }
    }
    else{
        try {
            QDir d2 = QFileInfo(activeirs).absoluteDir();
            ui->conv_dirpath->setText(d2.absolutePath());
            reloadIRS();
            if(ui->conv_files->count() >= 1){
                for(int i=0;i<ui->conv_files->count();i++){
                    if(ui->conv_files->item(i)->text()==QFileInfo(activeirs).fileName()){
                        ui->conv_files->setCurrentRow(i);
                        break;
                    }
                }
            }
        } catch (const exception& e) {
            LogHelper::error("Failed to load previous IRS path: " + QString::fromStdString(e.what()));
            ui->conv_dirpath->setText(m_appwrapper->getIrsPath());
            reloadIRS();
        }
    }

    //Reload Liveprog selection
    if(!QFile(activeliveprog).exists()){
        ui->liveprog_dirpath->setText(m_appwrapper->getLiveprogPath());
        reloadLiveprog();
    }else{
        try {
            QDir d2 = QFileInfo(activeliveprog).absoluteDir();
            ui->liveprog_dirpath->setText(d2.absolutePath());
            reloadLiveprog();
        } catch (const exception& e) {
            LogHelper::error("Failed to load previous Liveprog path: " + QString::fromStdString(e.what()));
            ui->liveprog_dirpath->setText(m_appwrapper->getLiveprogPath());
            reloadLiveprog();
        }
    }

    //Populate EQ preset list
    for(auto preset : PresetProvider::EQ::EQ_LOOKUP_TABLE().keys())
        ui->eqpreset->addItem(preset);

    ConnectActions();

    if(m_appwrapper->getTrayMode() || m_startupInTraySwitch) trayIcon->show();
    else trayIcon->hide();

    connect(m_dbus, &DBusProxy::propertiesCommitted, this, [this](){
        conf->setConfigMap(m_dbus->FetchPropertyMap());
        LoadConfig(Context::DBus);
    });

    connect(m_appwrapper,&AppConfigWrapper::styleChanged,this,[this](){
        ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
        ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
        ui->tabbar->redrawTabBar();
        RestartSpectrum();
        ui->eq_widget->setAccentColor(palette().highlight().color());
    });

    ui->eq_widget->setAlwaysDrawHandles(m_appwrapper->getEqualizerPermanentHandles());
    connect(m_appwrapper,&AppConfigWrapper::eqChanged,this,[this](){
        ui->eq_widget->setAlwaysDrawHandles(m_appwrapper->getEqualizerPermanentHandles());
    });

    ToggleSpectrum(m_appwrapper->getSpetrumEnable(),true);

    if(!m_appwrapper->getIntroShown())
        LaunchFirstRunSetup();
    else
        QTimer::singleShot(300,this,[this]{
            RunDiagnosticChecks();
        });

    ui->tabbar->setAnimatePageChange(true);
    ui->tabbar->setCustomStackWidget(ui->tabhost);
    ui->tabbar->setDetachCustomStackedWidget(true);
    ui->tabbar->addPage("Bass/Misc");
    ui->tabbar->addPage("Sound Positioning");
    ui->tabbar->addPage("Reverb");
    ui->tabbar->addPage("Equalizer");
    ui->tabbar->addPage("Compressor");
    ui->tabbar->addPage("Convolver");
    ui->tabbar->addPage("DDC");
    ui->tabbar->addPage("Liveprog");
    ui->tabbar->addPage("Graphic EQ");
    ui->frame->setStyleSheet(QString("QFrame#frame{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
    ui->tabhost->setStyleSheet(QString("QWidget#tabHostPage1,QWidget#tabHostPage2,QWidget#tabHostPage3,QWidget#tabHostPage4,QWidget#tabHostPage5,QWidget#tabHostPage6,QWidget#tabHostPage7,QWidget#tabHostPage8,QWidget#tabHostPage9{background-color: %1;}").arg(qApp->palette().window().color().lighter().name()));
    ui->tabbar->redrawTabBar();

    QTimer::singleShot(300,this,[this]{
        if(m_appwrapper->getLegacyTabs())
            InitializeLegacyTabs();
    });

    restoreGraphicEQView();
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::showEvent( QShowEvent* event ) {
    QWidget::showEvent( event );
    if(m_appwrapper->getLegacyTabs())
        InitializeLegacyTabs();
}
void MainWindow::InitializeLegacyTabs(){
    if(!ui->frame->isVisible())
        return;

    ui->frame->hide();
    ui->tabhost_legacy->show();
    for(int i = 1; i <= 7; i++){
        QWidget* w = findChild<QWidget*>(QString("tabHostPage%1").arg(i));
        replaceTab(ui->tabhost_legacy,i - 1, w);
        ui->tabhost_legacy->widget(i - 1)->setContentsMargins(9,9,9,9);
    }
    ui->tabhost_legacy->setCurrentIndex(0);
}
void MainWindow::LaunchFirstRunSetup(){
    FirstLaunchWizard* wiz = new FirstLaunchWizard(m_appwrapper,this);
    QHBoxLayout* lbLayout = new QHBoxLayout;
    QMessageOverlay* lightBox = new QMessageOverlay(this);
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect();
    lightBox->setGraphicsEffect(eff);
    lightBox->setLayout(lbLayout);
    lightBox->layout()->addWidget(wiz);
    lightBox->show();
    QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
    a->setDuration(500);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(wiz,&FirstLaunchWizard::wizardFinished,[=]{
        QPropertyAnimation *b = new QPropertyAnimation(eff,"opacity");
        b->setDuration(500);
        b->setStartValue(1);
        b->setEndValue(0);
        b->setEasingCurve(QEasingCurve::OutCirc);
        b->start(QPropertyAnimation::DeleteWhenStopped);
        connect(b,&QAbstractAnimation::finished, [=](){
            m_appwrapper->setIntroShown(true);
            lightBox->hide();
            settings_dlg->refreshAll();
            QTimer::singleShot(300,this,[this]{
                RunDiagnosticChecks();
            });
        });
    });
}
void MainWindow::RunDiagnosticChecks(){
#ifndef DISABLE_DIAGNOSTICS
    //Check if jdsp is correctly installed and running
    QFile pidfile("/tmp/jdsp4linux/pid.tmp");
    QString pid;
    if (pidfile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&pidfile);
        if (!stream.atEnd())
            pid = stream.readLine();
    }
    pidfile.close();
    if(system("which jdsp > /dev/null 2>&1") == 1){
        OverlayMsgProxy *msg = new OverlayMsgProxy(this);
        msg->openError(tr("JDSP not installed"),
                       tr("Unable to find jdsp executable.\n"
                          "Please make sure jdsp is installed and you\n"
                          "are using the lastest version of gst-plugin-jamesdsp"),
                       tr("Continue anyway"));
        LogHelper::error("JDSP binary not found ('which jdsp' returned 1)");
    }
    else if(pidfile.exists() && !m_dbus->isValid() &&
            system("kill -0 $(cat /tmp/jamesdsp/pid.tmp) > /dev/null")==0){
        OverlayMsgProxy *msg = new OverlayMsgProxy(this);
        msg->openError(tr("Unsupported version"),
                       tr("Looks like you are using an older version of\n"
                          "gst-plugin-jamesdsp. JDSP appears to be running\n"
                          "but no DBus interface has been found, so either the\n"
                          "DBus server was unable to launch and couldn't acquire a busname.\n"
                          "Please check the github repository and consider to update!"),
                       tr("Continue anyway"));
        LogHelper::error("JDSP is running but there is no DBus interface available (the gst-plugin is very likely outdated or was unable to register the DBus service)");
    }
    else if(!m_dbus->isValid())
        ShowDBusError();
    else
        CheckDBusVersion();
#endif
}

//Spectrum
void MainWindow::SetSpectrumVisibility(bool v){
    m_spectrograph->setVisible(v);
    if(v)
        this->findChild<QFrame*>("analysisLayout_spectrum")->setFrameShape(QFrame::StyledPanel);
    else
        this->findChild<QFrame*>("analysisLayout_spectrum")->setFrameShape(QFrame::NoFrame);
}
void MainWindow::InitializeSpectrum(){
    m_spectrograph = new Spectrograph(this);
    m_audioengine = new AudioStreamEngine(this);

    int refresh = m_appwrapper->getSpectrumRefresh();
    if(refresh == 0) refresh = 20;
    if(refresh < 10) refresh = 10;
    else if(refresh > 500) refresh = 500;
    m_audioengine->setNotifyIntervalMs(refresh);

    analysisLayout.reset(new QFrame());
    analysisLayout->setObjectName("analysisLayout_spectrum");
    analysisLayout->setFrameShape(QFrame::Shape::StyledPanel);
    analysisLayout->setLayout(new QHBoxLayout);
    analysisLayout->layout()->setMargin(0);
    analysisLayout->layout()->addWidget(m_spectrograph);

    auto buttonbox = ui->centralWidget->layout()->takeAt(ui->centralWidget->layout()->count()-1);
    ui->centralWidget->layout()->addWidget(analysisLayout.data());
    ui->centralWidget->layout()->addItem(buttonbox);
    analysisLayout.take();

    SetSpectrumVisibility(false);

    connect(m_appwrapper,&AppConfigWrapper::spectrumChanged,this,[this]{
        ToggleSpectrum(m_appwrapper->getSpetrumEnable(),true);
    });
    connect(m_appwrapper,&AppConfigWrapper::spectrumReloadRequired,this,&MainWindow::RestartSpectrum);
}
void MainWindow::RestartSpectrum(){
    ToggleSpectrum(false,false);
    ToggleSpectrum(m_appwrapper->getSpetrumEnable(),false);
}
void MainWindow::RefreshSpectrumParameters(){
    int bands = m_appwrapper->getSpectrumBands();
    int minfreq = m_appwrapper->getSpectrumMinFreq();
    int maxfreq = m_appwrapper->getSpectrumMaxFreq();
    int refresh = m_appwrapper->getSpectrumRefresh();
    float multiplier = m_appwrapper->getSpectrumMultiplier();
    //Set default values if undefined
    if(bands == 0) bands = 100;
    if(maxfreq == 0) maxfreq = 1000;
    if(refresh == 0) refresh = 10;
    if(multiplier == 0) multiplier = 0.15;

    //Check boundaries
    if(bands < 5 ) bands = 5;
    else if(bands > 300) bands = 300;
    if(minfreq < 0) minfreq = 0;
    else if(minfreq > 10000) minfreq = 10000;
    if(maxfreq < 100) maxfreq = 100;
    else if(maxfreq > 24000) maxfreq = 24000;
    if(refresh < 10) refresh = 10;
    else if(refresh > 500) refresh = 500;
    if(multiplier < 0.01) multiplier = 0.01;
    else if(multiplier > 1) multiplier = 1;

    if(maxfreq < minfreq) maxfreq = minfreq + 100;

    QColor outline;
    if (palette().window().style() == Qt::TexturePattern)
        outline = QColor(0, 0, 0, 160);
    else
        outline = palette().window().color().lighter(140);

    if(m_appwrapper->getSpectrumTheme() == 0)
        m_spectrograph->setTheme(Qt::black,
                                 QColor(51,204,201),
                                 QColor(51,204,201).darker(),
                                 QColor(255,255,0),
                                 m_appwrapper->getSpetrumGrid(),
                                 (Spectrograph::Mode)m_appwrapper->getSpectrumShape());
    else
        m_spectrograph->setTheme(palette().window().color().lighter(),
                                 palette().highlight().color(),
                                 palette().text().color(),
                                 outline.lighter(108),
                                 m_appwrapper->getSpetrumGrid(),
                                 (Spectrograph::Mode)m_appwrapper->getSpectrumShape());

    m_spectrograph->setParams(bands, minfreq, maxfreq);
    m_audioengine->setNotifyIntervalMs(refresh);
    m_audioengine->setMultiplier(multiplier);
}
void MainWindow::ToggleSpectrum(bool on,bool ctrl_visibility){
    RefreshSpectrumParameters();
    if(ctrl_visibility)spectrum->setVisible(on);
    if(on && (!m_spectrograph->isVisible() || !ctrl_visibility)){
        if(ctrl_visibility){
            SetSpectrumVisibility(true);
            this->setFixedSize(this->width(),this->height()+m_spectrograph->size().height());
        }

        QAudioDeviceInfo in;
        for(auto item : QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
            if(item.deviceName()==m_appwrapper->getSpectrumInput())
                in = item;

        LogHelper::debug("Spectrum Expected Input Device: "+m_appwrapper->getSpectrumInput());
        LogHelper::debug("Spectrum Found Input Device: "+in.deviceName());
        LogHelper::debug("Spectrum Default Input Device: "+QAudioDeviceInfo::defaultInputDevice().deviceName());

        m_audioengine->setAudioInputDevice(in);
        m_audioengine->initializeRecord();
        m_audioengine->startRecording();

        connect(m_audioengine, static_cast<void (AudioStreamEngine::*)(QAudio::Mode,QAudio::State)>(&AudioStreamEngine::stateChanged),
                this, [this](QAudio::Mode mode,QAudio::State state){
            Q_UNUSED(mode);

            if (QAudio::ActiveState != state && QAudio::SuspendedState != state) {
                m_spectrograph->reset();
            }
        });

        connect(m_audioengine, static_cast<void (AudioStreamEngine::*)(qint64, qint64, const FrequencySpectrum &)>(&AudioStreamEngine::spectrumChanged),
                this, [this](qint64, qint64,const FrequencySpectrum &spectrum){
            m_spectrograph->spectrumChanged(spectrum);
        });
    }
    else if(!on && (m_spectrograph->isVisible() || !ctrl_visibility)){
        if(ctrl_visibility){
            SetSpectrumVisibility(false);
            this->setFixedSize(this->width(),this->height()-m_spectrograph->size().height());
        }
        m_spectrograph->reset();
        m_audioengine->reset();
    }
}
//Overrides
void MainWindow::setVisible(bool visible)
{
    //Reconnect to dbus to make sure the connection isn't stale
    m_dbus = new DBusProxy();
    updateTrayPresetList();
    updateTrayConvolverList();
    //Hide all other windows if set to invisible
    if(!visible){
        log_dlg->hide();
        preset_dlg->hide();
    }
    if(m_dbus->isValid() &&
            msg_notrunning != nullptr)
        msg_notrunning->hide();
    QMainWindow::setVisible(visible);
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveGraphicEQView();
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}
//DBus
void MainWindow::ShowDBusError(){
#ifndef DISABLE_DIAGNOSTICS
    if(msg_notrunning != nullptr)
        msg_notrunning->hide();
    msg_notrunning = new OverlayMsgProxy(this);
    msg_notrunning->openError(tr("JDSP not running"),
                              tr("Unable to connect to DBus interface.\n"
                                 "Please make sure jdsp is running and you are\n"
                                 "using the lastest version of gst-plugin-jamesdsp"),
                              tr("Launch now"));
    LogHelper::error("DBus interface unavailable");
    connect(msg_notrunning,&OverlayMsgProxy::buttonPressed,[this](){
        int returncode = system("jdsp start");
        if(returncode != 0){
            if(msg_launchfail != nullptr)
                msg_launchfail->hide();
            QTimer::singleShot(300,this,[this,returncode](){
                msg_launchfail = new OverlayMsgProxy(this);
                msg_launchfail->openError(tr("Failed to launch JDSP"),
                                          tr("jdsp.sh has returned a non-null exit code.\n"
                                             "Please make sure jdsp is correctly installed\n"
                                             "and try to restart it manually"),
                                          tr("Continue anyway"));
                LogHelper::error(QString("Could not start jdsp.sh (returned %1)").arg(returncode));
            });
        } else {
            //Reconnect DBus
            QTimer::singleShot(500,this,[this](){
                m_dbus = new DBusProxy();
                RestartSpectrum();
            });
        }
    });
#endif
}
void MainWindow::CheckDBusVersion(){
    VersionContainer currentPluginVersion(m_dbus->GetVersion());
    VersionContainer minimumPluginVersion(QString(MINIMUM_PLUGIN_VERSION));
    if(currentPluginVersion < minimumPluginVersion){
        if(msg_versionmismatch != nullptr)
            msg_versionmismatch->hide();
        msg_versionmismatch = new OverlayMsgProxy(this);
        msg_versionmismatch->openError(tr("Version unsupported"),
                                       tr("This app requires a different version\n"
                                          "of gst-plugin-jamesdsp to function correctly.\n"
                                          "Consider to update gst-plugin-jamesdsp and/or\n"
                                          "this GUI in order to ensure full functionality.\n"
                                          "Current version: %1, Required version: >=%2").arg(currentPluginVersion).arg(MINIMUM_PLUGIN_VERSION),
                                       tr("Close"));
        LogHelper::error(QString("GStreamer plugin version mismatch (current: %1, required: >= %2)").arg(currentPluginVersion).arg(MINIMUM_PLUGIN_VERSION));
    }
}
//Systray
void MainWindow::raiseWindow(){
    Qt::WindowFlags eFlags = this->windowFlags();
    eFlags |= Qt::WindowStaysOnTopHint;
    this->setWindowFlags(eFlags);
    this->show();
    eFlags &= ~Qt::WindowStaysOnTopHint;
    this->setWindowFlags(eFlags);
    this->showNormal();
    this->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    this->raise();
    this->activateWindow();
}
void MainWindow::setTrayVisible(bool visible){
    if(visible) trayIcon->show();
    else trayIcon->hide();
}
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        setVisible(!this->isVisible());
        if(isVisible()){
            this->showNormal();
            this->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            this->raise();
            this->activateWindow();
        }
        //Hide tray icon if disabled and MainWin is visible (for cmdline force switch)
        if(!m_appwrapper->getTrayMode() && this->isVisible()) trayIcon->hide();
        break;
    default:
        ;
    }
}
void MainWindow::updateTrayPresetList(){
    if(tray_presetMenu != nullptr){
        tray_presetMenu->clear();
        QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
        QString path = pathAppend(absolute,"presets");

        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");

        QStringList nameFilter("*.conf");
        QStringList files = dir.entryList(nameFilter);
        if(files.count()<1){
            QAction *noPresets = new QAction("No presets found");
            noPresets->setEnabled(false);
            tray_presetMenu->addAction(noPresets);
        }
        else{
            for(int i = 0; i < files.count(); i++){
                //Strip extensions
                QFileInfo fi(files[i]);
                files[i] = fi.completeBaseName();
                //Add entry
                QAction *newEntry = new QAction(files[i]);
                connect(newEntry,&QAction::triggered,this,[=](){
                    LoadPresetFile(pathAppend(path,QString("%1.conf").arg(files[i])));
                });
                tray_presetMenu->addAction(newEntry);
            }
        }
    }
}
void MainWindow::updateTrayConvolverList(){
    if(tray_convMenu != nullptr){
        tray_convMenu->clear();
        QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
        QString path = pathAppend(absolute,"irs_favorites");

        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");

        QStringList nameFilter({"*.wav","*.irs"});
        QStringList files = dir.entryList(nameFilter);
        if(files.count()<1){
            QAction *noPresets = new QAction("No impulse responses found");
            noPresets->setEnabled(false);
            tray_convMenu->addAction(noPresets);
        }
        else{
            for(int i = 0; i < files.count(); i++){
                //Add entry
                QAction *newEntry = new QAction(files[i]);
                connect(newEntry,&QAction::triggered,this,[=](){
                    SetIRS(files[i],true);
                });
                tray_convMenu->addAction(newEntry);
            }
        }
    }
}
void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip("JamesDSP for Linux");
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    trayIcon->setIcon(QIcon(":/icons/icon.png"));
}
void MainWindow::updateTrayMenu(QMenu* menu){
    trayIcon->hide();
    createTrayIcon();
    trayIcon->show();
    trayIconMenu = menu;
    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIcon->contextMenu(),&QMenu::aboutToShow,[this]{
        updateTrayPresetList();
        updateTrayConvolverList();
    });
    m_appwrapper->setTrayContextMenu(MenuIO::buildString(menu));
}
QMenu* MainWindow::getTrayContextMenu(){
    return trayIconMenu;
}
void MainWindow::initGlobalTrayActions(){
    tray_disableAction = new QAction(tr("&Disable FX"), this);
    tray_disableAction->setProperty("tag","disablefx");
    tray_disableAction->setCheckable(true);
    tray_disableAction->setChecked(!conf->getBool("enable"));
    connect(tray_disableAction, &QAction::triggered, this, [this](){
        conf->setValue("enable",!tray_disableAction->isChecked());
        ui->disableFX->setChecked(tray_disableAction->isChecked());
        ApplyConfig();
    });
    tray_presetMenu = new QMenu(tr("&Presets"));
    tray_presetMenu->setProperty("tag","menu_preset");
    tray_convMenu = new QMenu(tr("&Convolver Bookmarks"));
    tray_convMenu->setProperty("tag","menu_convolver");
    auto init = MenuIO::buildMenu(buildAvailableActions(),m_appwrapper->getTrayContextMenu());
    if(init->actions().count() < 1)
        init = buildDefaultActions();
    updateTrayMenu(init);
}
QMenu* MainWindow::buildAvailableActions()
{
    QAction *reloadAction = new QAction(tr("&Reload JamesDSP"), this);
    reloadAction->setProperty("tag","reload");
    connect(reloadAction, &QAction::triggered, this, &MainWindow::Restart);

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    quitAction->setProperty("tag","quit");

    QMenu* reverbMenu = new QMenu(tr("Re&verberation Presets"), this);
    for(auto preset : PresetProvider::Reverb::getPresetNames()){
        QAction *newEntry = new QAction(preset);
        connect(newEntry,&QAction::triggered,this,[=](){
            ui->roompresets->setCurrentText(preset);
            ReverbPresetSelectionUpdated();
        });
        reverbMenu->addAction(newEntry);
    }
    reverbMenu->setProperty("tag","menu_reverb_preset");

    QMenu* eqMenu = new QMenu(tr("&EQ Presets"), this);
    for(auto preset : PresetProvider::EQ::EQ_LOOKUP_TABLE().keys()){
        QAction *newEntry = new QAction(preset);
        connect(newEntry,&QAction::triggered,this,[=](){
            if(preset == "Default")
                ResetEQ();
            else{
                ui->eqpreset->setCurrentText(preset);
                EqPresetSelectionUpdated();
            }
        });
        eqMenu->addAction(newEntry);
    }
    eqMenu->setProperty("tag","menu_eq_preset");

    QMenu* stereoWidenerMenu = new QMenu(tr("Stereo &Widener Presets"), this);
    for(auto preset : PresetProvider::StereoWidener::SW_LOOKUP_TABLE().keys()){
        if(preset == "Unknown") continue;
        QAction *newEntry = new QAction(preset);
        connect(newEntry,&QAction::triggered,this,[=](){
            ui->stereowide_preset->setCurrentText(preset);
            StereoWidePresetSelectionUpdated();
        });
        stereoWidenerMenu->addAction(newEntry);
    }
    stereoWidenerMenu->setProperty("tag","menu_stereowidener_preset");

    QMenu* bs2bMenu = new QMenu(tr("&BS2B Presets"), this);
    for(auto preset : PresetProvider::BS2B::BS2B_LOOKUP_TABLE().keys()){
        if(preset == "Unknown") continue;
        QAction *newEntry = new QAction(preset);
        connect(newEntry,&QAction::triggered,this,[=](){
            ui->bs2b_preset_cb->setCurrentText(preset);
            BS2BPresetSelectionUpdated();
        });
        bs2bMenu->addAction(newEntry);
    }
    bs2bMenu->setProperty("tag","menu_bs2b_preset");

    QMenu* menu = new QMenu(this);
    menu->addAction(tray_disableAction);
    menu->addAction(reloadAction);
    menu->addMenu(tray_presetMenu);
    menu->addSeparator();
    menu->addMenu(reverbMenu);
    menu->addMenu(eqMenu);
    menu->addMenu(stereoWidenerMenu);
    menu->addMenu(bs2bMenu);
    menu->addMenu(tray_convMenu);
    menu->addSeparator();
    menu->addAction(quitAction);
    return menu;
}
QMenu* MainWindow::buildDefaultActions()
{
    QAction *reloadAction = new QAction(tr("&Reload JamesDSP"), this);
    reloadAction->setProperty("tag","reload");
    connect(reloadAction, &QAction::triggered, this, &MainWindow::Restart);

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    quitAction->setProperty("tag","quit");

    QMenu* menu = new QMenu(this);
    menu->addAction(tray_disableAction);
    menu->addAction(reloadAction);
    menu->addMenu(tray_presetMenu);
    menu->addSeparator();
    menu->addAction(quitAction);
    return menu;
}

//---Dialogs/Buttons
void MainWindow::DialogHandler(){
    if(sender() == ui->set){
        QFrame* host = new QFrame(this);
        host->setProperty("menu", false);
        QVBoxLayout* hostLayout = new QVBoxLayout(host);
        hostLayout->addWidget(settings_dlg);
        host->hide();
        host->setAutoFillBackground(true);
        settings_dlg->updateInputSinks();

        connect(settings_dlg,&SettingsDlg::closeClicked,this,[host](){
            host->update();
            host->repaint();
            WAF::Animation::sideSlideOut(host, WAF::BottomSide);
        });

        WAF::Animation::sideSlideIn(host, WAF::BottomSide);
    }
    else if(sender() == ui->cpreset){
        if(preset_dlg->isVisible()){
            //Hacky workaround to reliably raise the window on all distros
            Qt::WindowFlags eFlags = preset_dlg->windowFlags();
            eFlags |= Qt::WindowStaysOnTopHint;
            preset_dlg->setWindowFlags(eFlags);
            preset_dlg->show();
            eFlags &= ~Qt::WindowStaysOnTopHint;
            preset_dlg->setWindowFlags(eFlags);
            preset_dlg->showNormal();
            preset_dlg->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            preset_dlg->raise();
            preset_dlg->activateWindow();
            return;
        }

        preset_dlg->move(x() + (width() - preset_dlg->width()) / 2,
                         y() + (height() - preset_dlg->height()) / 2);

        preset_dlg->show();
    }
}
void MainWindow::OpenLog(){
    log_dlg->show();
    log_dlg->updateLog();
}
void MainWindow::Reset(){
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,tr("Reset Configuration"),tr("Are you sure?"),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        std::filebuf fb;
        fb.open (m_appwrapper->getPath().toUtf8().constData(),std::ios::out);
        std::ostream os(&fb);
        os << default_config;
        fb.close();

        conf->setConfigMap(readConfig());
        LoadConfig();
        m_irsNeedUpdate = true;

        ApplyConfig();
    }
}
void MainWindow::DisableFX(){
    tray_disableAction->setChecked(ui->disableFX->isChecked());
    //Apply instantly
    if(!lockapply)ApplyConfig();
}
void MainWindow::ForceReload(){
    conf->setConfigMap(readConfig());
    LoadConfig();
    m_irsNeedUpdate = true;
    ApplyConfig();
}

//---Reloader
void MainWindow::OnUpdate(bool ignoremode){
    //Will be called when slider has been moved, dynsys/eq preset set or spinbox changed
    if((m_appwrapper->getAutoFx() &&
        (ignoremode||m_appwrapper->getAutoFxMode()==0)) && !lockapply)
        ApplyConfig();
}
void MainWindow::OnRelease(){
    if((m_appwrapper->getAutoFx() &&
        m_appwrapper->getAutoFxMode()==1) && !lockapply)
        ApplyConfig();
}
void MainWindow::Restart(){
    if(m_appwrapper->getMuteOnRestart())system("pactl set-sink-mute 0 1");
    if(m_appwrapper->getGFix())system("killall -r glava");
    system("jdsp restart");
    if(m_appwrapper->getGFix())system("setsid glava -d >/dev/null 2>&1 &");
    if(m_appwrapper->getMuteOnRestart())system("pactl set-sink-mute 0 0");
    RestartSpectrum();
}

//---User preset management
void MainWindow::LoadPresetFile(const QString& filename){
    const QString& src = filename;
    const QString dest = m_appwrapper->getPath();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::debug("Loading from " + filename+ " (main/loadpreset)");
    conf->setConfigMap(readConfig());
    LoadConfig();
    m_irsNeedUpdate = true;

    ApplyConfig();
}
void MainWindow::SavePresetFile(const QString& filename){
    const QString src = m_appwrapper->getPath();
    const QString& dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::debug("Saving to " + filename+ " (main/savepreset)");
}
void MainWindow::LoadExternalFile(){
    QString filename = QFileDialog::getOpenFileName(this,tr("Load custom audio.conf"),"","*.conf");
    if(filename=="")return;
    const QString& src = filename;
    const QString dest = m_appwrapper->getPath();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::debug("Loading from " + filename+ " (main/loadexternal)");
    conf->setConfigMap(readConfig());
    LoadConfig();
    m_irsNeedUpdate = true;

    ApplyConfig();
}
void MainWindow::SaveExternalFile(){
    QString filename = QFileDialog::getSaveFileName(this,tr("Save current audio.conf"),"","*.conf");
    if(filename=="")return;
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if(ext!="conf")filename.append(".conf");

    const QString src = m_appwrapper->getPath();
    const QString dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::debug("Saving to " + filename+ " (main/saveexternal)");
}

//---Config IO
void MainWindow::LoadConfig(Context ctx){
    lockapply=true;
    tray_disableAction->setChecked(!conf->getBool("enable"));
    ui->disableFX->setChecked(!conf->getBool("enable"));

    ui->analog->setChecked(conf->getBool("analogmodelling_enable"));
    ui->analog_tubedrive->setValueA(conf->getInt("analogmodelling_tubedrive"));

    ui->bassboost->setChecked(conf->getBool("bass_enable"));
    ui->bass_sensitivity->setValueA((int)(conf->getFloat("bass_boostcond")*100.f));
    ui->bass_maxgain->setValueA(conf->getInt("bass_maxgain"));
    ui->bass_gaintime->setValueA(conf->getInt("bass_gainsmooth"));
    ui->bass_spectraltime->setValueA(conf->getInt("bass_spectralsmooth"));

    ui->reverb->setChecked(conf->getBool("headset_enable"));
    ui->rev_osf->setValueA(conf->getInt("headset_osf"));
    ui->rev_era->setValueA(100*conf->getInt("headset_reflection_amount"));
    ui->rev_erw->setValueA(100*conf->getInt("headset_reflection_width"));
    ui->rev_erf->setValueA(100*conf->getInt("headset_reflection_factor"));
    ui->rev_finaldry->setValueA(10*conf->getInt("headset_finaldry"));
    ui->rev_finalwet->setValueA(10*conf->getInt("headset_finalwet"));
    ui->rev_width->setValueA(100*conf->getInt("headset_width"));
    ui->rev_wet->setValueA(10 *conf->getInt("headset_wet"));
    ui->rev_bass->setValueA(100*conf->getInt("headset_bassboost"));
    ui->rev_spin->setValueA(100*conf->getInt("headset_lfo_spin"));
    ui->rev_wander->setValueA(100*conf->getInt("headset_lfo_wander"));
    ui->rev_decay->setValueA(100*conf->getInt("headset_decay"));
    ui->rev_delay->setValueA(10*conf->getInt("headset_delay"));
    ui->rev_lci->setValueA(conf->getInt("headset_lpf_input"));
    ui->rev_lcb->setValueA(conf->getInt("headset_lpf_bass"));
    ui->rev_lcd->setValueA(conf->getInt("headset_lpf_damp"));
    ui->rev_lco->setValueA(conf->getInt("headset_lpf_output"));

    ui->stereowidener->setChecked(conf->getBool("stereowide_enable"));
    ui->stereowide_m->setValueA(conf->getInt("stereowide_mcoeff"));
    ui->stereowide_s->setValueA(conf->getInt("stereowide_scoeff"));

    ui->bs2b->setChecked(conf->getBool("bs2b_enable"));
    ui->bs2b_feed->setValueA(conf->getInt("bs2b_feed"));
    ui->bs2b_fcut->setValueA(conf->getInt("bs2b_fcut"));

    ui->enable_comp->setChecked(conf->getBool("compression_enable"));
    ui->comp_lowthres->setValueA(conf->getInt("compression_thres1"));
    ui->comp_highthres->setValueA(conf->getInt("compression_thres2"));
    ui->comp_maxattack->setValueA(conf->getInt("compression_maxatk"));
    ui->comp_maxrelease->setValueA(conf->getInt("compression_maxrel"));
    ui->comp_pregain->setValueA(conf->getInt("compression_pregain"));

    ui->limthreshold->setValueA(conf->getInt("masterswitch_limthreshold"));
    ui->limrelease->setValueA(conf->getInt("masterswitch_limrelease"));
    ui->postgain->setValueA(conf->getInt("masterswitch_postgain"));

    ui->graphicEq->chk_enable->setChecked(conf->getBool("streq_enable"));
    QString streq = conf->getString("streq_stringp",false);
    if(streq.size() > 2){
        if(streq.at(0)=='"')streq.remove(0,1); //remove double quotes
        if(streq.at(streq.length()-1)=='"')streq.chop(1);
        ui->graphicEq->load(streq);
    }

    ui->ddc_enable->setChecked(conf->getBool("ddc_enable"));
    QString ddc = conf->getString("ddc_file",false);
    if(ddc.size() > 2){
        if(ddc.at(0)=='"')ddc.remove(0,1); //remove double quotes
        if(ddc.at(ddc.length()-1)=='"')ddc.chop(1);
        activeddc = ddc;
    }

    ui->liveprog_enable->setChecked(conf->getBool("liveprog_enable"));
    QString liveprog = conf->getString("liveprog_file",false);
    if(liveprog.size() > 2){
        if(liveprog.at(0)=='"')liveprog.remove(0,1); //remove double quotes
        if(liveprog.at(liveprog.length()-1)=='"')liveprog.chop(1);
        activeliveprog = liveprog;
    }

    ui->conv_enable->setChecked(conf->getBool("convolver_enable"));
    QString ir = conf->getString("convolver_file",false);
    if(ir.size() > 2){
        if(ir.at(0)=='"')ir.remove(0,1); //remove double quotes
        if(ir.at(ir.length()-1)=='"')ir.chop(1);
        activeirs = ir;
    }

    ui->enable_eq->setChecked(conf->getBool("tone_enable"));
    ui->eqinterpolator->setCurrentIndex(conf->getInt("tone_interpolation"));
    ui->eqfiltertype->setCurrentIndex(conf->getInt("tone_filtertype"));

    //*** Parse EQ String to QMap
    QString rawEqString = chopFirstLastChar(conf->getString("tone_eq"));
    bool isOldFormat = rawEqString.split(";").count() == 15;

    if(isOldFormat)
        rawEqString = "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;"
                + rawEqString;

    QVector<float> rawEqData;
    for(auto val : rawEqString.split(";"))
        if(!val.isEmpty())
            if(isOldFormat)
                rawEqData.push_back(val.toFloat() / 100.f);
            else
                rawEqData.push_back(val.toFloat());
        else
            rawEqData.push_back(0.f);

    QMap<float,float> eqData;
    QVector<float> dbData;
    for(int i = 0; i < rawEqData.size(); i++){
        if(i <= 14)
            if((i + 15) < rawEqData.size())
                eqData[rawEqData.at(i)] = rawEqData.at(i + 15);
            else
                eqData[rawEqData.at(i)] = 0.0;
        else
            dbData.push_back(rawEqData.at(i));
    }

    // Decide if fixed or flexible EQ should be enabled
    if(rawEqString.contains("25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0")){
        //Use fixed 15-band EQ
        setEQMode(0);

        int it = 0;
        bool eqReloadRequired = false;
        for(auto cur_data : ui->eq_widget->getBands()){
            if(it >= rawEqData.count())
                break;

            bool equal = isApproximatelyEqual<float>(cur_data,dbData.at(it));
            if(eqReloadRequired == false)
                eqReloadRequired = !equal;
            it++;
        }
        if(eqReloadRequired)
            ui->eq_widget->setBands(dbData,false);
    } else {
        //Use flexible 15-band EQ
        setEQMode(1);
        ui->eq_dyn_widget->loadMap(eqData);
    }

    if(ctx != Context::DBus){
        UpdateEqStringFromWidget();
        UpdateStereoWideStringFromWidget();
        UpdateBS2BStringFromWidget();
    }
    UpdateAllUnitLabels();

    lockapply=false;
}
void MainWindow::ApplyConfig(bool restart){
    conf->setValue("enable",QVariant(!ui->disableFX->isChecked()));

    conf->setValue("analogmodelling_enable",QVariant(ui->analog->isChecked()));
    conf->setValue("analogmodelling_tubedrive",QVariant(ui->analog_tubedrive->valueA()));
    conf->setValue("masterswitch_limthreshold",QVariant(ui->limthreshold->valueA()));
    conf->setValue("masterswitch_limrelease",QVariant(ui->limrelease->valueA()));
    conf->setValue("masterswitch_postgain",QVariant(ui->postgain->valueA()));

    conf->setValue("ddc_enable",QVariant(ui->ddc_enable->isChecked()));
    conf->setValue("ddc_file",QVariant("\"" + activeddc + "\""));

    conf->setValue("liveprog_enable",QVariant(ui->liveprog_enable->isChecked()));
    conf->setValue("liveprog_file",QVariant("\"" + activeliveprog + "\""));

    conf->setValue("convolver_enable",QVariant(ui->conv_enable->isChecked()));
    conf->setValue("convolver_file",QVariant("\"" + activeirs + "\""));

    conf->setValue("compression_enable",QVariant(ui->enable_comp->isChecked()));
    conf->setValue("compression_thres1",QVariant(ui->comp_lowthres->valueA()));
    conf->setValue("compression_thres2",QVariant(ui->comp_highthres->valueA()));
    conf->setValue("compression_maxatk",QVariant(ui->comp_maxattack->valueA()));
    conf->setValue("compression_maxrel",QVariant(ui->comp_maxrelease->valueA()));
    conf->setValue("compression_pregain",QVariant(ui->comp_pregain->valueA()));

    conf->setValue("tone_enable",QVariant(ui->enable_eq->isChecked()));
    conf->setValue("tone_filtertype",QVariant(ui->eqfiltertype->currentIndex()));
    conf->setValue("tone_interpolation",QVariant(ui->eqinterpolator->currentIndex()));

    if(ui->eq_r_fixed->isChecked()){
        QVector<float> eqBands = ui->eq_widget->getBands();
        QString rawEqString = "25.0;40.0;63.0;100.0;160.0;250.0;400.0;630.0;1000.0;1600.0;2500.0;4000.0;6300.0;10000.0;16000.0;";
        int counter = 0;
        for(auto band : eqBands){
            rawEqString.append(QString::number(band));
            if(counter < 14)
                rawEqString.append(';');
            counter++;
        }
        conf->setValue("tone_eq",QVariant("\"" + rawEqString + "\""));
    } else {
        QString rawEqString;
        ui->eq_dyn_widget->storeCsv(rawEqString);
        conf->setValue("tone_eq",QVariant("\"" + rawEqString + "\""));
    }

    conf->setValue("bass_enable",QVariant(ui->bassboost->isChecked()));

    conf->setValue("bass_boostcond",QVariant(ui->bass_sensitivity->valueA()/100.f));
    conf->setValue("bass_maxgain",QVariant(ui->bass_maxgain->valueA()));
    conf->setValue("bass_gainsmooth",QVariant(ui->bass_gaintime->valueA()));
    conf->setValue("bass_spectralsmooth",QVariant(ui->bass_spectraltime->valueA()));

    conf->setValue("stereowide_enable",QVariant(ui->stereowidener->isChecked()));
    conf->setValue("stereowide_scoeff",QVariant(ui->stereowide_s->valueA()));
    conf->setValue("stereowide_mcoeff",QVariant(ui->stereowide_m->valueA()));

    conf->setValue("bs2b_enable",QVariant(ui->bs2b->isChecked()));
    conf->setValue("bs2b_feed",QVariant(ui->bs2b_feed->valueA()));
    conf->setValue("bs2b_fcut",QVariant(ui->bs2b_fcut->valueA()));

    conf->setValue("headset_enable",QVariant(ui->reverb->isChecked()));
    conf->setValue("headset_osf",QVariant(ui->rev_osf->valueA()));
    conf->setValue("headset_lpf_input",QVariant(ui->rev_lci->valueA()));
    conf->setValue("headset_lpf_bass",QVariant(ui->rev_lcb->valueA()));
    conf->setValue("headset_lpf_damp",QVariant(ui->rev_lcd->valueA()));
    conf->setValue("headset_lpf_output",QVariant(ui->rev_lco->valueA()));
    conf->setValue("headset_reflection_amount",QVariant(ui->rev_era->valueA()/100.0f));
    conf->setValue("headset_reflection_width",QVariant(ui->rev_erw->valueA()/100.0f));
    conf->setValue("headset_reflection_factor",QVariant(ui->rev_erf->valueA()/100.0f));
    conf->setValue("headset_finaldry",QVariant(ui->rev_finaldry->valueA()/10.0f));
    conf->setValue("headset_finalwet",QVariant(ui->rev_finalwet->valueA()/10.0f));
    conf->setValue("headset_width",QVariant(ui->rev_width->valueA()/100.0f));
    conf->setValue("headset_wet",QVariant(ui->rev_wet->valueA()/10.0f));
    conf->setValue("headset_bassboost",QVariant(ui->rev_bass->valueA()/100.0f));
    conf->setValue("headset_lfo_spin",QVariant(ui->rev_spin->valueA()/100.0f));
    conf->setValue("headset_lfo_wander",QVariant(ui->rev_wander->valueA()/100.0f));
    conf->setValue("headset_decay",QVariant(ui->rev_decay->valueA()/100.0f));
    conf->setValue("headset_delay",QVariant(ui->rev_delay->valueA()/10.0f));


    conf->setValue("streq_enable",QVariant(ui->graphicEq->chk_enable->isChecked()));
    QString streq;
    ui->graphicEq->store(streq);
    conf->setValue("streq_stringp",QVariant("\"" + streq + "\""));

    ConfigIO::writeFile(m_appwrapper->getPath(),conf->getConfigMap());

    ConfigContainer dbus_template = *conf;
    dbus_template.setValue("convolver_file",activeirs);
    m_dbus->SubmitPropertyMap(dbus_template.getConfigMap());
    if(restart){
        if(conf->getString("convolver_file",false).contains('$') &&
                conf->getBool("conv_enable",false) &&
                m_irsNeedUpdate)
            Restart();
        else
            if(m_appwrapper->getReloadMethod() == ReloadMethod::DIRECT_DBUS){
                if(!m_dbus->isValid())
                    ShowDBusError();
                else
                    m_dbus->CommitProperties(DBusProxy::PARAM_GROUP_ALL);
            }
            else
                Restart();
        m_irsNeedUpdate = false;
    }
}

//---Predefined Presets
void MainWindow::EqPresetSelectionUpdated(){
    if(ui->eqpreset->currentText() == "Custom")
        return;
    auto preset = PresetProvider::EQ::lookupPreset(ui->eqpreset->currentText());
    if(preset.size() > 0)SetEQ(preset);
    else ResetEQ();
}
void MainWindow::StereoWidePresetSelectionUpdated(){
    if(ui->stereowide_preset->currentText() == "...")
        return;
    const auto data = PresetProvider::StereoWidener::lookupPreset(ui->stereowide_preset->currentText());
    if(data.size() <= 1)
        return;
    lockapply=true;
    ui->stereowide_m->setValueA((int)(data.begin()[0]*1000.0f));
    ui->stereowide_s->setValueA((int)(data.begin()[1]*1000.0f));
    lockapply=false;
    UpdateAllUnitLabels();
    OnUpdate(true);
}
void MainWindow::BS2BPresetSelectionUpdated(){
    if(ui->bs2b_preset_cb->currentText() == "...")
        return;
    const auto data = PresetProvider::BS2B::lookupPreset(ui->bs2b_preset_cb->currentText());
    if(data.size() <= 1)
        return;
    lockapply=true;
    ui->bs2b_fcut->setValueA(data.begin()[0]);
    ui->bs2b_feed->setValueA(data.begin()[1]);
    lockapply=false;
    UpdateAllUnitLabels();
    OnUpdate(true);
}
void MainWindow::ReverbPresetSelectionUpdated(){
    if(ui->roompresets->currentText() == "...")
        return;
    const auto data = PresetProvider::Reverb::lookupPreset(ui->roompresets->currentIndex());
    SetReverbData(data);
}

//---Status
void MainWindow::UpdateUnitLabel(int d,QObject *alt){
    if(lockapply&&alt==nullptr)return;//Skip if lockapply-flag is set (when setting presets, ...)

    QObject* obj;

    if(alt==nullptr)obj = sender();
    else obj = alt;

    QString pre = "";
    QString post = "";
    if(obj==ui->stereowide_m||obj==ui->stereowide_s){
        UpdateTooltipLabelUnit(obj,QString::number((double)d/1000 )+"x",alt==nullptr);
    }
    else if(obj==ui->rev_width){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d )+"%",alt==nullptr);
    }
    else if(obj==ui->bs2b_feed){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/10 )+"dB",alt==nullptr);
    }
    else if(obj==ui->bass_sensitivity){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/100 ),alt==nullptr);
    }
    else if(obj==ui->analog_tubedrive){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/100 )+"dB",alt==nullptr);
    }
    else if(obj==ui->rev_decay){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/100 ),alt==nullptr);
    }
    else if(obj==ui->rev_delay){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/10 )+"ms",alt==nullptr);
    }
    else if(obj==ui->rev_wet||obj==ui->rev_finalwet||obj==ui->rev_finaldry){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/10 )+"dB",alt==nullptr);
    }
    else if(obj==ui->rev_era||obj==ui->rev_erf||obj==ui->rev_erw
            ||obj==ui->rev_width||obj==ui->rev_bass||obj==ui->rev_spin
            ||obj==ui->rev_wander){
        UpdateTooltipLabelUnit(obj,QString::number( (double)d/100 ),alt==nullptr);
    }
    else if(obj==ui->eqfiltertype||obj==ui->eqinterpolator
            ||obj==ui->bs2b_preset_cb){
        // Ignore these UI elements
    }
    else{
        if(obj==ui->comp_lowthres||obj==ui->comp_highthres||
                obj==ui->comp_pregain||obj==ui->postgain)
            post = "dB";
        else if(obj==ui->comp_maxattack||obj==ui->comp_maxrelease||obj==ui->limrelease)
            post = "ms";
        else if(obj==ui->bass_maxgain)post = "dB";
        else if(obj==ui->bass_gaintime
                ||obj==ui->bass_spectraltime)post = "ms";
        else if(obj==ui->bs2b_fcut)post = "Hz";
        else if(obj==ui->rev_lcb||obj==ui->rev_lcd
                ||obj==ui->rev_lci||obj==ui->rev_lco)post = "Hz";
        else if(obj==ui->rev_osf)post = "x";
        UpdateTooltipLabelUnit(obj,pre + QString::number(d) + post,alt==nullptr);
    }
    if(!lockapply||obj!=nullptr)OnUpdate(false);

}
void MainWindow::UpdateTooltipLabelUnit(QObject* sender,const QString& text,bool viasignal){
    QWidget *w = qobject_cast<QWidget*>(sender);
    w->setToolTip(text);
    if(viasignal)ui->info->setText(text);
}
void MainWindow::UpdateAllUnitLabels(){

    QList<QComboBox*> comboboxesToBeUpdated({ui->eqfiltertype,ui->eqinterpolator});

    QList<QAnimatedSlider*> slidersToBeUpdated(
    {ui->analog_tubedrive,ui->stereowide_m,ui->stereowide_s,ui->bs2b_fcut,ui->bs2b_feed,
     ui->limthreshold,ui->limrelease,ui->comp_maxrelease,ui->comp_maxattack,ui->comp_highthres,ui->comp_lowthres,
     ui->rev_osf,ui->rev_erf,ui->rev_era,ui->rev_erw,ui->rev_lci,ui->rev_lcb,ui->rev_lcd,
     ui->rev_lco,ui->rev_finalwet,ui->rev_finaldry,ui->rev_wet,ui->rev_width,ui->rev_spin,ui->rev_wander,ui->rev_decay,
     ui->rev_delay,ui->rev_bass,ui->postgain,ui->comp_pregain,ui->bass_maxgain,ui->bass_gaintime,ui->bass_sensitivity,ui->bass_spectraltime});

    foreach (auto w, slidersToBeUpdated)
        UpdateUnitLabel(w->valueA(),w);

    foreach (auto w, comboboxesToBeUpdated)
        UpdateUnitLabel(w->currentIndex(),w);
}

//---DDC
void MainWindow::reloadDDC(){
    lockddcupdate=true;
    QDir path(ui->ddc_dirpath->text());
    QStringList nameFilter("*.vdc");
    nameFilter.append("*.ddc");
    QStringList files = path.entryList(nameFilter);
    ui->ddc_files->clear();
    if(files.empty()){
        QFont font;
        font.setItalic(true);
        //font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("No VDC files found");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->ddc_files->addItem(placeholder);
    }
    else ui->ddc_files->addItems(files);

    if(ui->ddc_files->count() >= 1){
        for(int i=0;i<ui->ddc_files->count();i++){
            if(ui->ddc_files->item(i)->text()==QFileInfo(activeddc).fileName()){
                ui->ddc_files->setCurrentRow(i);
                break;
            }
        }
    }
    lockddcupdate=false;
}
void MainWindow::reloadDDCDB(){
    QJsonTableModel::Header header;
    header.push_back( QJsonTableModel::Heading( { {"title","Company"},   {"index","Company"} }) );
    header.push_back( QJsonTableModel::Heading( { {"title","Model"}, {"index","Model"} }) );
    header.push_back( QJsonTableModel::Heading( { {"title","SR_44100_Coeffs"}, {"index","SR_44100_Coeffs"} }) );
    header.push_back( QJsonTableModel::Heading( { {"title","SR_48000_Coeffs"}, {"index","SR_48000_Coeffs"} }) );

    model = new QJsonTableModel( header, this );
    ui->ddcTable->setModel(model);

    QFile file(":/ddc/DDCData.json");

    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream instream(&file);
        QJsonDocument jsonDocument = QJsonDocument::fromJson(instream.readAll().toLocal8Bit());
        model->setJson( jsonDocument );
    }

    model->setHeaderData(0, Qt::Horizontal, tr("Company"));
    model->setHeaderData(1, Qt::Horizontal, tr("Model"));
    model->setHeaderData(5, Qt::Horizontal, tr("SR_44100_Coeffs"));
    model->setHeaderData(6, Qt::Horizontal, tr("SR_48000_Coeffs"));

    ui->ddcTable->setModel(model);
    ui->ddcTable->setColumnHidden(2, true);
    ui->ddcTable->setColumnHidden(3, true);
    ui->ddcTable->resizeColumnsToContents();
}

//---IRS
void MainWindow::reloadIRS(){
    lockirsupdate=true;
    QDir path(ui->conv_dirpath->text());
    QStringList nameFilter("*.irs");
    nameFilter.append("*.wav");
    nameFilter.append("*.flac");
    QStringList files = path.entryList(nameFilter);
    ui->conv_files->clear();
    if(files.empty()){
        QFont font;
        font.setItalic(true);
        //font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("No IRS files found");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->conv_files->addItem(placeholder);
    }
    else ui->conv_files->addItems(files);
    lockirsupdate=false;
}
void MainWindow::reloadIRSFav(){
    lockirsupdate=true;
    QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
    QDir path(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites"));
    QStringList nameFilter("*.wav");
    nameFilter.append("*.irs");
    nameFilter.append("*.flac");
    QStringList files = path.entryList(nameFilter);
    ui->conv_files_fav->clear();
    if(files.empty()){
        QFont font;
        font.setItalic(true);
        //font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("Nothing here yet...");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->conv_files_fav->addItem(placeholder);
        QListWidgetItem* placeholder2 = new QListWidgetItem;
        //placeholder2->setFont(font);
        placeholder2->setText("Bookmark some IRS files in the 'filesystem' tab");
        placeholder2->setFlags(placeholder2->flags() & ~Qt::ItemIsEnabled);
        ui->conv_files_fav->addItem(placeholder2);
    }
    else ui->conv_files_fav->addItems(files);
    lockirsupdate=false;
}

//---Liveprog
void MainWindow::reloadLiveprog(){
    lockliveprogupdate=true;
    QDir path(ui->liveprog_dirpath->text());
    QStringList nameFilter("*.eel");
    QStringList files = path.entryList(nameFilter);
    ui->liveprog_files->clear();
    if(files.empty()){
        QFont font;
        font.setItalic(true);
        //font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("No EEL files found");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->liveprog_files->addItem(placeholder);
    }
    else ui->liveprog_files->addItems(files);

    if(ui->liveprog_files->count() >= 1){
        for(int i=0;i<ui->liveprog_files->count();i++){
            if(ui->liveprog_files->item(i)->text()==QFileInfo(activeliveprog).fileName()){
                ui->liveprog_files->setCurrentRow(i);
                setLiveprogSelection(activeliveprog);
                break;
            }
        }
    }
    lockliveprogupdate=false;
}
void MainWindow::setLiveprogSelection(QString path){
    ui->liveprog_name->setText(QFileInfo(path).fileName());
}

//---Helper
void MainWindow::SetEQ(const QVector<float>& data){
    lockapply=true;
    ui->eq_widget->setBands(QVector<float>(data));
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::ResetEQ(){
    ui->reseteq->setEnabled(false);
    QTimer::singleShot(510,this,[this](){
        ui->reseteq->setEnabled(true);
    });
    ui->eqpreset->setCurrentIndex(0);
    ui->eq_dyn_widget->load(DEFAULT_GRAPHICEQ);
    SetEQ(PresetProvider::EQ::defaultPreset());
}
void MainWindow::SetIRS(const QString& irs,bool apply){
    if(activeirs != irs) m_irsNeedUpdate = true;
    activeirs = irs;
    QFileInfo irsInfo(irs);
    //ui->convpath->setText(irsInfo.baseName());
    //ui->convpath->setCursorPosition(0);
    if(apply)ApplyConfig();
}
void MainWindow::SetReverbData(PresetProvider::Reverb::sf_reverb_preset_data data) {
    lockapply=true;
    ui->rev_osf->setValueA(data.osf);
    ui->rev_era->setValueA((int)(data.p1*100));
    ui->rev_finalwet->setValueA((int)(data.p2*10));
    ui->rev_finaldry->setValueA((int)(data.p3*10));
    ui->rev_erf->setValueA((int)(data.p4*100));
    ui->rev_erw->setValueA((int)(data.p5*100));
    ui->rev_width->setValueA((int)(data.p6*100));
    ui->rev_wet->setValueA((int)(data.p7*10));
    ui->rev_wander->setValueA((int)(data.p8*100));
    ui->rev_bass->setValueA((int)(data.p9*100));
    ui->rev_spin->setValueA((int)(data.p10*100));
    ui->rev_lci->setValueA((int)data.p11);
    ui->rev_lcb->setValueA((int)data.p12);
    ui->rev_lcd->setValueA((int)data.p13);
    ui->rev_lco->setValueA((int)data.p14);
    ui->rev_decay->setValueA((int)(data.p15*100));
    ui->rev_delay->setValueA((int)(data.p16*10));
    UpdateAllUnitLabels();
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::UpdateEqStringFromWidget(){
    QString currentEqPresetName =
            PresetProvider::EQ::reverseLookup(ui->eq_widget->getBands());

    ui->eqpreset->setCurrentText(currentEqPresetName);
}
void MainWindow::UpdateBS2BStringFromWidget(){
    QString currentBS2BPresetName =
            PresetProvider::BS2B::reverseLookup({ui->bs2b_feed->valueA(),ui->bs2b_fcut->valueA()});
    ui->bs2b_preset_cb->setCurrentText(currentBS2BPresetName);
}
void MainWindow::UpdateStereoWideStringFromWidget(){
    QString currentSWPresetName =
            PresetProvider::StereoWidener::reverseLookup({ui->stereowide_m->valueA()/1000.f,ui->stereowide_s->valueA()/1000.f});
    ui->stereowide_preset->setCurrentText(currentSWPresetName);
}

//---GraphicEQ States
void MainWindow::restoreGraphicEQView(){
    QVariantMap state;
    state = ConfigIO::readFile(m_appwrapper->getGraphicEQConfigFilePath());
    if(state.count() >= 1)
        ui->graphicEq->loadPreferences(state);
    else{
        ConfigContainer pref;
        pref.setValue("scrollX",165.346);
        pref.setValue("scrollY",75.7);
        pref.setValue("zoomX",0.561);
        pref.setValue("zoomY",0.713);
        ui->graphicEq->loadPreferences(pref.getConfigMap());
    }
}
void MainWindow::saveGraphicEQView(){
    QVariantMap state;
    ui->graphicEq->storePreferences(state);
    ConfigIO::writeFile(m_appwrapper->getGraphicEQConfigFilePath(),
                        state);
}

QVariantMap MainWindow::readConfig(){
    QVariantMap confmap = ConfigIO::readFile(m_appwrapper->getPath());
    if(confmap.count() < 1){
        //No audio.conf found
        std::filebuf fb;
        fb.open (m_appwrapper->getPath().toUtf8().constData(),std::ios::out);
        std::ostream os(&fb);
        os << default_config;
        fb.close();
        confmap = ConfigIO::readFile(m_appwrapper->getPath());
    }
    return confmap;
}
QString MainWindow::GetExecutablePath(){
    return m_exepath;
}
AppConfigWrapper* MainWindow::getACWrapper(){
    return m_appwrapper;
}

//---Connect UI-Signals
void MainWindow::ConnectActions(){    
    QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
    QList<QComboBox*> registerCurrentIndexChange({ui->eqfiltertype,ui->eqinterpolator});

    QList<QAnimatedSlider*> registerValueAChange(
    {ui->analog_tubedrive,ui->stereowide_m,ui->stereowide_s,ui->bs2b_fcut,ui->bs2b_feed,ui->bass_maxgain,ui->bass_gaintime,
     ui->bass_sensitivity,ui->bass_spectraltime,ui->limthreshold,ui->limrelease,ui->comp_maxrelease,ui->comp_maxattack,
     ui->rev_osf,ui->rev_erf,ui->rev_era,ui->rev_erw,ui->rev_lci,ui->rev_lcb,ui->rev_lcd,ui->comp_highthres,ui->comp_lowthres,
     ui->rev_lco,ui->rev_finalwet,ui->rev_finaldry,ui->rev_wet,ui->rev_width,ui->rev_spin,ui->rev_wander,ui->rev_decay,
     ui->rev_delay,ui->rev_bass,ui->postgain,ui->comp_pregain});

    QList<QWidget*> registerSliderRelease(
    {ui->stereowide_m,ui->stereowide_s,ui->bs2b_fcut,ui->bs2b_feed,ui->rev_osf,ui->rev_erf,ui->rev_era,ui->rev_erw,
     ui->rev_lci,ui->rev_lcb,ui->rev_lcd,ui->rev_lco,ui->rev_finalwet,ui->rev_finaldry,ui->rev_wet,ui->rev_width,ui->rev_spin,
     ui->rev_wander,ui->rev_decay,ui->rev_delay,ui->rev_bass,ui->analog_tubedrive,ui->limthreshold,
     ui->limrelease,ui->comp_maxrelease,ui->comp_maxattack,ui->comp_highthres,ui->comp_lowthres,
     ui->bass_maxgain,ui->bass_gaintime,ui->bass_sensitivity,ui->bass_spectraltime,ui->postgain,ui->comp_pregain});

    QList<QWidget*> registerClick(
    {ui->bassboost,ui->bs2b,ui->stereowidener,ui->analog,ui->reverb,ui->enable_eq,ui->enable_comp,ui->ddc_enable,ui->conv_enable,
     ui->graphicEq->chk_enable});

    QList<QWidget*> registerStereoWideUpdate(
    {ui->stereowide_m,ui->stereowide_s});

    QList<QWidget*> registerBS2BUpdate(
    {ui->bs2b_fcut,ui->bs2b_feed});

    foreach (QWidget* w, registerCurrentIndexChange)
        connect(w,                  SIGNAL(currentIndexChanged(int)),   this,   SLOT(UpdateUnitLabel(int)));

    foreach (QWidget* w, registerValueAChange)
        connect(w,                  SIGNAL(valueChangedA(int)),         this,   SLOT(UpdateUnitLabel(int)));

    foreach (QWidget* w, registerSliderRelease)
        connect(w,                  SIGNAL(sliderReleased()),           this,   SLOT(OnRelease()));

    foreach (QWidget* w, registerClick)
        connect(w,                  SIGNAL(clicked()),                  this,   SLOT(OnUpdate()));

    foreach (QWidget* w, registerStereoWideUpdate)
        connect(w,                  SIGNAL(sliderReleased()),           this,   SLOT(UpdateStereoWideStringFromWidget()));

    foreach (QWidget* w, registerBS2BUpdate)
        connect(w,                  SIGNAL(sliderReleased()),           this,   SLOT(UpdateBS2BStringFromWidget()));

    connect(ui->apply,              SIGNAL(clicked()),                  this,   SLOT(ApplyConfig()));
    connect(ui->disableFX,          SIGNAL(clicked()),                  this,   SLOT(DisableFX()));
    connect(ui->reseteq,            SIGNAL(clicked()),                  this,   SLOT(ResetEQ()));
    connect(ui->reset,              SIGNAL(clicked()),                  this,   SLOT(Reset()));
    connect(ui->conv_select,        SIGNAL(clicked()),                  this,   SLOT(DialogHandler()));
    connect(ui->cpreset,            SIGNAL(clicked()),                  this,   SLOT(DialogHandler()));
    connect(ui->set,                SIGNAL(clicked()),                  this,   SLOT(DialogHandler()));

    connect(ui->eqfiltertype,       SIGNAL(currentIndexChanged(int)),   this,   SLOT(OnRelease()));
    connect(ui->eqinterpolator,     SIGNAL(currentIndexChanged(int)),   this,   SLOT(OnRelease()));

    connect(ui->eq_widget,          SIGNAL(bandsUpdated()),             this,   SLOT(ApplyConfig()));
    connect(ui->eq_widget,          SIGNAL(mouseReleased()),            this,   SLOT(UpdateEqStringFromWidget()));
    connect(ui->eqpreset,           SIGNAL(currentIndexChanged(int)),   this,   SLOT(EqPresetSelectionUpdated()));
    connect(ui->stereowide_preset,  SIGNAL(currentIndexChanged(int)),   this,   SLOT(StereoWidePresetSelectionUpdated()));
    connect(ui->bs2b_preset_cb,     SIGNAL(currentIndexChanged(int)),   this,   SLOT(BS2BPresetSelectionUpdated()));
    connect(ui->roompresets,        SIGNAL(currentIndexChanged(int)),   this,   SLOT(ReverbPresetSelectionUpdated()));

    connect(ui->graphicEq,          SIGNAL(updateModel()),              this,   SLOT(OnUpdate()));

    connect(ui->ddc_reload,         SIGNAL(clicked()),                  this,   SLOT(reloadDDC()));
    connect(ui->ddc_files,          &QListWidget::itemSelectionChanged,[this]{
        if(lockddcupdate || ui->ddc_files->selectedItems().count() < 1)
            return; //Clearing Selection by code != User Interaction
        QString path = QDir(ui->ddc_dirpath->text()).filePath(ui->ddc_files->selectedItems().first()->text());
        if(QFileInfo::exists(path) && QFileInfo(path).isFile())
            activeddc = path;

        OnUpdate();
    });
    connect(ui->ddc_select,         &QPushButton::clicked,[this]{
        QFileDialog *fd = new QFileDialog;
        fd->setFileMode(QFileDialog::Directory);
        fd->setOption(QFileDialog::ShowDirsOnly);
        fd->setViewMode(QFileDialog::Detail);
        QString result = fd->getExistingDirectory();
        if (result!="")
        {
            ui->ddc_dirpath->setText(result);
            reloadDDC();
        }
    });
    connect(ui->ddcTable->selectionModel(), &QItemSelectionModel::selectionChanged,this,[this](const QItemSelection &, const QItemSelection &){
        QItemSelectionModel *select = ui->ddcTable->selectionModel();
        QString ddc_coeffs;
        if(select->hasSelection()){
            lockddcupdate=true;
            ui->ddc_files->clearSelection();
            lockddcupdate=false;
            int index = select->selectedRows().first().row();
            ddc_coeffs += "SR_44100:";
            ddc_coeffs += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index,2)).toString();
            ddc_coeffs += "\nSR_48000:";
            ddc_coeffs += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index,3)).toString();
            QString absolute = QFileInfo(m_appwrapper->getPath()).absoluteDir().absolutePath();
            QFile file(absolute+"/dbcopy.vdc");
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                file.write(ddc_coeffs.toUtf8().constData());
            file.close();

            activeddc = absolute + "/dbcopy.vdc";
        }
        OnUpdate();
    });

    connect(ui->liveprog_reload,    SIGNAL(clicked()),                  this,   SLOT(reloadLiveprog()));
    connect(ui->liveprog_files,     &QListWidget::itemSelectionChanged,[this]{
        if(lockliveprogupdate || ui->liveprog_files->selectedItems().count() < 1)
            return; //Clearing Selection by code != User Interaction
        QString path = QDir(ui->liveprog_dirpath->text()).filePath(ui->liveprog_files->selectedItems().first()->text());
        if(QFileInfo::exists(path) && QFileInfo(path).isFile())
            activeliveprog = path;

        setLiveprogSelection(activeliveprog);

        OnUpdate();
    });
    connect(ui->liveprog_select,    &QPushButton::clicked,[this]{
        QFileDialog *fd = new QFileDialog;
        fd->setFileMode(QFileDialog::Directory);
        fd->setOption(QFileDialog::ShowDirsOnly);
        fd->setViewMode(QFileDialog::Detail);
        QString result = fd->getExistingDirectory();
        if (result!="")
        {
            ui->liveprog_dirpath->setText(result);
            reloadLiveprog();
        }
    });

    connect(ui->conv_files,          &QListWidget::itemSelectionChanged,[this]{
        if(lockirsupdate || ui->conv_files->selectedItems().count() < 1)
            return; //Clearing Selection by code != User Interaction
        QString path = QDir(ui->conv_dirpath->text()).filePath(ui->conv_files->selectedItems().first()->text());
        if(QFileInfo::exists(path) && QFileInfo(path).isFile())
            activeirs = path;

        OnUpdate();
    });
    connect(ui->conv_select,         &QPushButton::clicked,[this]{
        QFileDialog *fd = new QFileDialog;
        fd->setFileMode(QFileDialog::Directory);
        fd->setOption(QFileDialog::ShowDirsOnly);
        fd->setViewMode(QFileDialog::Detail);
        QString result = fd->getExistingDirectory();
        if (result!="")
        {
            ui->conv_dirpath->setText(result);
            reloadIRS();
        }
    });
    connect(ui->conv_bookmark,       &QPushButton::clicked,[this,absolute]{
        if(ui->conv_files->selectedItems().count() < 1)
            return; //Clearing Selection by code != User Interaction

        const QString src = QDir(ui->conv_dirpath->text()).filePath(ui->conv_files->selectedItems().first()->text());
        const QString& dest = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files->selectedItems().first()->text());

        if (QFile::exists(dest))QFile::remove(dest);

        QFile::copy(src,dest);
        LogHelper::debug("Adding " + src + " to bookmarks (convolver/add)");
        reloadIRSFav();
    });
    connect(ui->conv_files_fav,      &QListWidget::itemSelectionChanged,[this,absolute]{
        if(lockirsupdate || ui->conv_files_fav->selectedItems().count() < 1)
            return; //Clearing Selection by code != User Interaction
        QString path = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());
        if(QFileInfo::exists(path) && QFileInfo(path).isFile())
            activeirs = path;

        OnUpdate();
    });
    connect(ui->conv_fav_rename,     &QPushButton::clicked,[this,absolute]{
        if(ui->conv_files_fav->selectedItems().count() < 1)return;
        bool ok;
        QString text = QInputDialog::getText(this, "Rename",
                                             "New Name", QLineEdit::Normal,
                                             ui->conv_files_fav->selectedItems().first()->text(), &ok);
        QString fullpath = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());;
        QString dest = QDir::cleanPath(absolute + QDir::separator() + "irs_favorites");
        if (ok && !text.isEmpty())
            QFile::rename(fullpath,QDir(dest).filePath(text));
        reloadIRSFav();
    });
    connect(ui->conv_fav_remove,     &QPushButton::clicked,[this,absolute]{
        if(ui->conv_files_fav->selectedItems().count() < 1)
            return;
        QString fullpath = QDir(QDir::cleanPath(absolute + QDir::separator() + "irs_favorites")).filePath(ui->conv_files_fav->selectedItems().first()->text());;
        if(!QFile::exists(fullpath)){
            QMessageBox::warning(this, "Error", "Selected File doesn't exist",QMessageBox::Ok);
            reloadIRSFav();
            return;
        }
        QFile file (fullpath);
        file.remove();
        LogHelper::debug("Removed "+fullpath+" from favorites (convolver/remove)");
        reloadIRSFav();
    });

    connect(ui->graphicEq,&GraphicEQFilterGUI::autoeqClicked,[this]{
        AutoEQSelector* sel = new AutoEQSelector(this);
        sel->setModal(true);
        if(sel->exec() == QDialog::Accepted){
            HeadphoneMeasurement hp = sel->getSelection();
            if(hp.getGraphicEQ() == "")
                QMessageBox::warning(this,"Error","Empty equalizer data.\n\nEither your network connection is experiencing issues, or you are being rate-limited by GitHub.\nKeep in mind that you can only send 60 web requests per hour to this API.\n\nYou can check your current rate limit status here: https://api.github.com/rate_limit");
            else{
                ui->graphicEq->load(hp.getGraphicEQ());
                OnUpdate();
            }
        }
        sel->deleteLater();
    });

    connect(ui->eq_r_fixed,&QRadioButton::clicked,this,&MainWindow::updateEQMode);
    connect(ui->eq_r_flex,&QRadioButton::clicked,this,&MainWindow::updateEQMode);

    connect(ui->liveprog_editscript, &QAbstractButton::clicked,[this]{
        if(activeliveprog.isEmpty()){
            QMessageBox::warning(this,"Error","No EEL file loaded.\n"
                                              "Please select one in the list on the left side.");
            return;
        } else if(!QFile(activeliveprog).exists()){
            QMessageBox::warning(this,"Error","Selected EEL file does not exist anymore.\n"
                                              "Please select another one");
            return;
        }
        m_eelEditor->show();
        m_eelEditor->openNewScript(activeliveprog);
    });
    connect(m_eelEditor,&EELEditor::scriptSaved,this,&MainWindow::reloadLiveprog);
}

void MainWindow::updateEQMode(){
    bool isFixed = ui->eq_r_fixed->isChecked();
    setEQMode(isFixed ? 0 : 1);
}

void MainWindow::setEQMode(int mode){
    ui->eq_holder->setCurrentIndex(mode);
    ui->eqpreset->setEnabled(!mode);
    ui->eq_r_flex->setChecked(mode);
    ui->eq_r_fixed->setChecked(!mode);
}
