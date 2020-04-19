#include "settingsdlg.h"
#include "ui_settings.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "palettedlg.h"
#include "misc/autostartmanager.h"
#include "pulseeffectscompatibility.h"

#include <QGraphicsOpacityEffect>
#include <QProcess>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QTimer>

using namespace std;
static bool lockslot = false;

SettingsDlg::SettingsDlg(MainWindow* mainwin,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settings){
    ui->setupUi(this);
    m_mainwin = mainwin;

    appconf = mainwin->getACWrapper();
    QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

    /*
     * Prepare TreeView
     */
    ui->selector->setCurrentItem(ui->selector->findItems("General",Qt::MatchFlag::MatchExactly).first());
    ui->stackedWidget->setCurrentIndex(0);
    connect(ui->selector,static_cast<void (QTreeWidget::*)(QTreeWidgetItem*,QTreeWidgetItem*)>(&QTreeWidget::currentItemChanged),this,[this](QTreeWidgetItem* cur, QTreeWidgetItem*){
        int toplevel_index = ui->selector->indexOfTopLevelItem(cur);
        switch(toplevel_index){
        case -1:
            if(cur->text(0) == "Context Menu")
                ui->stackedWidget->setCurrentIndex(5);
            if(cur->text(0) == "Design")
                ui->stackedWidget->setCurrentIndex(7);
            if(cur->text(0) == "Advanced")
                ui->stackedWidget->setCurrentIndex(8);
            break;
        case 5:
            // -- SA/ROOT
            ui->stackedWidget->setCurrentIndex(6);
            break;
        default:
            ui->stackedWidget->setCurrentIndex(toplevel_index);
        }
    });
    ui->selector->expandItem(ui->selector->findItems("Spectrum Analyser",Qt::MatchFlag::MatchExactly).first());
    ui->selector->expandItem(ui->selector->findItems("Systray",Qt::MatchFlag::MatchExactly).first());

    /*
     * Prepare all combooxes
     */
    ui->styleSelect->addItem("Default","default");
    ui->styleSelect->addItem("MacOS","aqua");
    ui->styleSelect->addItem("Ubuntu","ubuntu");
    ui->paletteSelect->addItem("Default","default");
    ui->paletteSelect->addItem("Black","black");
    ui->paletteSelect->addItem("Blue","blue");
    ui->paletteSelect->addItem("Dark","dark");
    ui->paletteSelect->addItem("Dark Blue","darkblue");
    ui->paletteSelect->addItem("Dark Green","darkgreen");
    ui->paletteSelect->addItem("Honeycomb","honeycomb");
    ui->paletteSelect->addItem("Gray","gray");
    ui->paletteSelect->addItem("Green","green");
    ui->paletteSelect->addItem("Stone","stone");
    ui->paletteSelect->addItem("Custom","custom");
    for ( const auto& i : QStyleFactory::keys() )
        ui->themeSelect->addItem(i);

    /*
     * Refresh all input fields
     */
    refreshAll();

    /*
     * Connect all signals for page General
     */
    connect(ui->savepath, &QPushButton::clicked, this, [this]{
        appconf->setPath(ui->path->text());
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("Restart required"), tr("Please restart this application to make sure all changes are applied correctly.\n"
                                                                       "Press 'OK' to quit or 'Cancel' if you want to continue without a restart."),
                                      QMessageBox::Ok|QMessageBox::Cancel);
        if (reply == QMessageBox::Ok)
            QApplication::quit();
    });
    connect(ui->glavafix, &QPushButton::clicked, this, [this]{
        appconf->setGFix(ui->glavafix->isChecked());
    });
    connect(ui->muteonrestart, &QCheckBox::clicked, this, [this]{
        appconf->setMuteOnRestart(ui->muteonrestart->isChecked());
    });
    connect(ui->autofx, &QGroupBox::clicked, this, [this]{
        appconf->setAutoFx(ui->autofx->isChecked());
    });
    connect(ui->run_first_launch, &QPushButton::clicked, this, [this,mainwin]{
        emit closeClicked();
        QTimer::singleShot(500,this,[mainwin]{
            mainwin->LaunchFirstRunSetup();
        });
    });
    auto autofx_mode = [this]{
        if(lockslot)return;
        int mode = 0;
        if(ui->aa_instant->isChecked())mode=0;
        else if(ui->aa_release->isChecked())mode=1;
        appconf->setAutoFxMode(mode);
    };
    connect(ui->aa_instant, &QRadioButton::clicked, this, autofx_mode);
    connect(ui->aa_release, &QRadioButton::clicked, this, autofx_mode);
    connect(ui->reloadmethod,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](){
        if(lockslot)return;
        appconf->setReloadMethod((ReloadMethod)ui->reloadmethod->currentIndex());
    });
    connect(ui->glavafix_help, &QPushButton::clicked, this, [this]{
        QMessageBox::information(this,tr("Help"),
                                 tr("This fix kills GLava (desktop visualizer) and restarts it after a new config has been applied.\nThis prevents GLava to switch to another audio sink, while V4L is restarting."));
    });
    /*
     * Connect all signals for Session
     */
    auto systray_sel = [this,mainwin]{
        if(lockslot)return;
        int mode = 0;
        if(ui->systray_r_none->isChecked())mode=0;
        else if(ui->systray_r_showtray->isChecked())mode=1;
        appconf->setTrayMode(mode);
        mainwin->setTrayVisible(mode);
        ui->systray_icon_box->setEnabled(mode);
        ui->menu_edit->setEnabled(mode);
    };
    connect(ui->systray_r_none,&QRadioButton::clicked,this,systray_sel);
    connect(ui->systray_r_showtray,&QRadioButton::clicked,this,systray_sel);
    auto autostart_update = [this,mainwin,autostart_path]{
        if(ui->systray_minOnBoot->isChecked()){
            AutostartManager::saveDesktopFile(autostart_path,mainwin->GetExecutablePath(),
                                              ui->systray_autostartJDSP->isChecked(),
                                              ui->systray_delay->isChecked());
        }
        else QFile(autostart_path).remove();
        ui->systray_autostartJDSP->setEnabled(ui->systray_minOnBoot->isChecked());
        ui->systray_delay->setEnabled(ui->systray_minOnBoot->isChecked());
    };
    connect(ui->systray_minOnBoot,&QPushButton::clicked,this,autostart_update);
    connect(ui->systray_autostartJDSP,&QPushButton::clicked,this,autostart_update);
    connect(ui->systray_delay,&QPushButton::clicked,this,autostart_update);
    /*
     * Connect all signals for Interface
     */
    auto change_theme_mode = [this]{
        if(lockslot)return;
        int mode = 0;
        if(ui->uimode_css->isChecked())mode=0;
        else if(ui->uimode_pal->isChecked())mode=1;

        ui->styleSelect->setEnabled(!mode);
        ui->paletteSelect->setEnabled(mode);
        ui->paletteConfig->setEnabled(mode && appconf->getColorpalette()=="custom");
        appconf->setThememode(mode);
    };
    connect(ui->uimode_css, &QRadioButton::clicked, this, change_theme_mode);
    connect(ui->uimode_pal, &QRadioButton::clicked, this, change_theme_mode);
    connect(ui->themeSelect, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
            this, [this](const QString&){
        if(lockslot)return;
        appconf->setTheme(ui->themeSelect->itemText(ui->themeSelect->currentIndex()).toUtf8().constData());
    });
    connect(ui->styleSelect,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),this,[this]{
        if(lockslot)return;
        appconf->setStylesheet(ui->styleSelect->itemData(ui->styleSelect->currentIndex()).toString());
    });
    connect(ui->paletteSelect,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),this,[this]{
        if(lockslot)return;
        appconf->setColorpalette(ui->paletteSelect->itemData(ui->paletteSelect->currentIndex()).toString());
        ui->paletteConfig->setEnabled(appconf->getColorpalette()=="custom");
    });
    connect(ui->paletteConfig,&QPushButton::clicked,this,[this]{
        auto c = new class PaletteEditor(appconf,this);
        c->setFixedSize(c->geometry().width(),c->geometry().height());
        c->show();
    });
    connect(ui->eq_alwaysdrawhandles,&QCheckBox::clicked,[this](){
        appconf->setEqualizerPermanentHandles(ui->eq_alwaysdrawhandles->isChecked());
    });
    connect(ui->legacytabs,&QCheckBox::clicked,[this](){
        appconf->setLegacyTabs(ui->legacytabs->isChecked());
        if(ui->legacytabs->isChecked())
            m_mainwin->InitializeLegacyTabs();
        else{
            QMessageBox::StandardButton reply =
                    QMessageBox::question(this, tr("Restart required"), tr("Please restart this application to make sure all changes are applied correctly.\n"
                                                                           "Press 'OK' to quit or 'Cancel' if you want to continue without a restart."),
                                          QMessageBox::Ok|QMessageBox::Cancel);
            if (reply == QMessageBox::Ok)
                QApplication::quit();
        }
    });
    /*
     * Connect all signals for Default Paths
     */
    connect(ui->saveirspath, &QPushButton::clicked, this, [this]{
        appconf->setIrsPath(ui->irspath->text());
    });
    connect(ui->saveddcpath, &QPushButton::clicked, this, [this]{
        appconf->setDDCPath(ui->ddcpath->text());
    });
    connect(ui->saveliveprogpath, &QPushButton::clicked, this, [this]{
        appconf->setLiveprogPath(ui->saveliveprogpath->text());
    });
    connect(ui->liveprog_autoextract,&QCheckBox::clicked,[this](){
        appconf->setLiveprogAutoExtract(ui->liveprog_autoextract->isChecked());
    });
    connect(ui->liveprog_extractNow, &QPushButton::clicked, this, [this,mainwin]{
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,tr("Question"),tr("Do you want to override existing EEL scripts (if any)?"),
                                      QMessageBox::Yes|QMessageBox::No);

        int count = mainwin->extractDefaultEELScripts(reply == QMessageBox::Yes);
        if(count > 0)
            QMessageBox::information(this,"Extract scripts",QString("%1 script(s) have been restored").arg(count));
        else
            QMessageBox::information(this,"Extract scripts",QString("No scripts have been extracted"));
    });

    /*
     * Connect all signals for Devices
     */
    auto deviceUpdated = [this](){
        if(lockslot) return;
        QString absolute =
                QFileInfo(appconf->getPath()).absoluteDir().absolutePath();
        QString devices(pathAppend(absolute,"devices.conf"));
        if(ui->dev_mode_auto->isChecked()){
            QFile(devices).remove();
        }else{
            if(ui->dev_select->currentData() == "---")
                return;

            ConfigContainer* devconf = new ConfigContainer();
            devconf->setConfigMap(ConfigIO::readFile(devices));
            devconf->setValue("location",ui->dev_select->currentData());
            ConfigIO::writeFile(devices,devconf->getConfigMap());
        }
    };
    connect(ui->dev_reload_jdsp,&QPushButton::clicked,mainwin,&MainWindow::Restart);
    connect(ui->dev_mode_auto,&QRadioButton::clicked,this,deviceUpdated);
    connect(ui->dev_mode_manual,&QRadioButton::clicked,this,deviceUpdated);
    connect(ui->dev_select,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, deviceUpdated);
    connect(ui->dev_pe_compat, &QPushButton::clicked, this, [this]{
        showPECompatibilityScreen();
    });
    /*
     * Connect all signals for SA/ROOT
     */
    connect(ui->sa_enable,&QGroupBox::clicked,this,[this,mainwin](){
        appconf->setSpectrumEnable(ui->sa_enable->isChecked());
        ui->spectrum_theme->setEnabled(ui->sa_enable->isChecked());
        ui->spectrum_advanced->setEnabled(ui->sa_enable->isChecked());
        emit closeClicked();
        mainwin->ui->set->click();
    });
    connect(ui->sa_bands,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,[this](int number){
        appconf->setSpectrumBands(number);
    });
    connect(ui->sa_minfreq,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,[this](int number){
        appconf->setSpectrumMinFreq(number);
    });
    connect(ui->sa_maxfreq,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,[this](int number){
        appconf->setSpectrumMaxFreq(number);
    });
    connect(ui->sa_input,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, [this](const QString& str){
        if(lockslot)return;
        appconf->setSpectrumInput(str);
    });
    connect(ui->sa_type,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, [this](const QString&){
        if(lockslot)return;
        appconf->setSpectrumShape(ui->sa_type->currentIndex());
    });
    /*
     * Connect all signals for SA/Design
     */
    connect(ui->sa_grid,&QCheckBox::clicked,this,[this](){
        appconf->setSpectrumGrid(ui->sa_grid->isChecked());
    });
    auto sa_theme_sel = [this]{
        int mode = 0;
        if(ui->sa_theme_default->isChecked())mode=0;
        else if(ui->sa_theme_inherit->isChecked())mode=1;
        appconf->setSpectrumTheme(mode);
    };
    connect(ui->sa_theme_default,&QRadioButton::clicked,this,sa_theme_sel);
    connect(ui->sa_theme_inherit,&QRadioButton::clicked,this,sa_theme_sel);

    /*
     * Connect all signals for SA/Advanced
     */
    connect(ui->sa_refresh,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,[this](int number){
        appconf->setSpectrumRefresh(number);
    });
    connect(ui->sa_multi,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),this,[this](double number){
        appconf->setSpectrumMultiplier(number);
    });


    /*
     * Connect all signals for Global
     */
    connect(ui->close, &QPushButton::clicked, this, &SettingsDlg::closeClicked);
    connect(ui->github, &QPushButton::clicked, this, []{
        QDesktopServices::openUrl(QUrl("https://github.com/Audio4Linux/JDSP4Linux-GUI"));
    });
    connect(ui->menu_edit,&QMenuEditor::targetChanged,[mainwin,this]{
        auto menu = ui->menu_edit->exportMenu();
        mainwin->updateTrayMenu(menu);
    });
    connect(ui->menu_edit,&QMenuEditor::resetPressed,[mainwin,this]{
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Warning", "Do you really want to restore the default layout?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ui->menu_edit->setTargetMenu(mainwin->buildDefaultActions());
            auto menu = ui->menu_edit->exportMenu();
            mainwin->updateTrayMenu(menu);
        }
    });
    ui->menu_edit->setSourceMenu(mainwin->buildAvailableActions());

    /*
     * Check for systray availability
     */
#ifndef QT_NO_SYSTEMTRAYICON
    ui->systray_unsupported->hide();
#else
    ui->session->setEnabled(false);
#endif
    if(!QSystemTrayIcon::isSystemTrayAvailable()){
        ui->systray_unsupported->show();
        ui->session->setEnabled(false);
    }
}
SettingsDlg::~SettingsDlg(){
    delete ui;
}
void SettingsDlg::refreshDevices()
{
    lockslot = true;
    ui->dev_select->clear();
    QString absolute =
            QFileInfo(appconf->getPath()).absoluteDir().absolutePath();
    QFile devices(pathAppend(absolute,"devices.conf"));
    bool devmode_auto = !devices.exists();
    ui->dev_mode_auto->setChecked(devmode_auto);
    ui->dev_mode_manual->setChecked(!devmode_auto);

    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("sh", QStringList()<<"-c"<<"pactl list sinks | grep \'Name: \' -A1");
    process.waitForFinished(500);

    ConfigContainer* devconf = new ConfigContainer();
    devconf->setConfigMap(ConfigIO::readFile(pathAppend(absolute,"devices.conf")));
    QString out = process.readAllStandardOutput();
    ui->dev_select->addItem("...","---");
    for(auto item : out.split("Name:")){
        item.prepend("Name:");
        QRegularExpression re(R"((?<=(Name:)\s)(?<name>.+)[\s\S]+(?<=(Description:)\s)(?<desc>.+))");
        QRegularExpressionMatch match = re.match(item, 0, QRegularExpression::PartialPreferCompleteMatch);
        if(match.hasMatch()){
            ui->dev_select->addItem(QString("%1 (%2)").arg(match.captured("desc")).arg(match.captured("name")),
                                    match.captured("name"));
        }
    }
    QString dev_location = devconf->getVariant("location",true).toString();
    if(dev_location.isEmpty())
        ui->dev_select->setCurrentIndex(0);
    else{
        bool notFound = true;
        for(int i = 0; i < ui->dev_select->count(); i++){
            if(ui->dev_select->itemData(i) ==
                    dev_location){
                notFound = false;
                ui->dev_select->setCurrentIndex(i);
                break;
            }
        }
        if(notFound){
            QString name = QString("Unknown (%1)").arg(dev_location);
            ui->dev_select->addItem(name,dev_location);
            ui->dev_select->setCurrentText(name);
        }
    }
    lockslot = false;
}

void SettingsDlg::refreshAll(){
    lockslot = true;
    QString autostart_path = AutostartManager::getAutostartPath("jdsp-gui.desktop");

    ui->menu_edit->setTargetMenu(m_mainwin->getTrayContextMenu());
    ui->menu_edit->setIconStyle(appconf->getWhiteIcons());

    ui->path->setText(appconf->getPath());
    ui->irspath->setText(appconf->getIrsPath());
    ui->ddcpath->setText(appconf->getDDCPath());
    ui->liveprog_path->setText(appconf->getLiveprogPath());
    ui->autofx->setChecked(appconf->getAutoFx());
    ui->muteonrestart->setChecked(appconf->getMuteOnRestart());
    ui->glavafix->setChecked(appconf->getGFix());
    ui->reloadmethod->setCurrentIndex((int)appconf->getReloadMethod());

    ui->liveprog_autoextract->setChecked(appconf->getLiveprogAutoExtract());

    updateInputSinks();

    QString qvT(appconf->getTheme());
    int indexT = ui->themeSelect->findText(qvT);
    if ( indexT != -1 ) {
        ui->themeSelect->setCurrentIndex(indexT);
    }else{
        int index_fallback = ui->themeSelect->findText("Fusion");
        if ( index_fallback != -1 )
            ui->themeSelect->setCurrentIndex(index_fallback);
    }

    QVariant qvS(appconf->getStylesheet());
    int index = ui->styleSelect->findData(qvS);
    if ( index != -1 )
        ui->styleSelect->setCurrentIndex(index);

    QVariant qvS2(appconf->getColorpalette());
    int index2 = ui->paletteSelect->findData(qvS2);
    if ( index2 != -1 )
        ui->paletteSelect->setCurrentIndex(index2);

    ui->styleSelect->setEnabled(!appconf->getThememode());
    ui->paletteConfig->setEnabled(appconf->getThememode() && appconf->getColorpalette()=="custom");
    ui->paletteSelect->setEnabled(appconf->getThememode());

    ui->uimode_css->setChecked(!appconf->getThememode());//If 0 set true, else false
    ui->uimode_pal->setChecked(appconf->getThememode());//If 0 set false, else true

    ui->aa_instant->setChecked(!appconf->getAutoFxMode());//same here..
    ui->aa_release->setChecked(appconf->getAutoFxMode());

    ui->systray_r_none->setChecked(!appconf->getTrayMode());
    ui->systray_r_showtray->setChecked(appconf->getTrayMode());
    ui->systray_icon_box->setEnabled(appconf->getTrayMode());
    ui->menu_edit->setEnabled(appconf->getTrayMode());

    bool autostart_enabled = AutostartManager::inspectDesktopFile(autostart_path,AutostartManager::Exists);
    bool autostartjdsp_enabled = AutostartManager::inspectDesktopFile(autostart_path,AutostartManager::UsesJDSPAutostart);
    bool autostart_delayed = AutostartManager::inspectDesktopFile(autostart_path,AutostartManager::Delayed);

    ui->systray_minOnBoot->setChecked(autostart_enabled);
    ui->systray_autostartJDSP->setEnabled(autostart_enabled);
    ui->systray_autostartJDSP->setChecked(autostartjdsp_enabled);
    ui->systray_delay->setEnabled(autostart_enabled);
    ui->systray_delay->setChecked(autostart_delayed);

    ui->eq_alwaysdrawhandles->setChecked(appconf->getEqualizerPermanentHandles());
    ui->legacytabs->setChecked(appconf->getLegacyTabs());

    refreshDevices();

    int bands = appconf->getSpectrumBands();
    int minfreq = appconf->getSpectrumMinFreq();
    int maxfreq = appconf->getSpectrumMaxFreq();
    int refresh = appconf->getSpectrumRefresh();
    float multiplier = appconf->getSpectrumMultiplier();
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
    ui->sa_enable->setChecked(appconf->getSpetrumEnable());
    ui->spectrum_theme->setEnabled(appconf->getSpetrumEnable());
    ui->spectrum_advanced->setEnabled(appconf->getSpetrumEnable());

    ui->sa_type->setCurrentIndex(appconf->getSpectrumShape());
    ui->sa_bands->setValue(bands);
    ui->sa_minfreq->setValue(minfreq);
    ui->sa_maxfreq->setValue(maxfreq);
    ui->sa_grid->setChecked(appconf->getSpetrumGrid());
    ui->sa_refresh->setValue(refresh);
    ui->sa_multi->setValue(multiplier);

    ui->sa_theme_default->setChecked(!appconf->getSpectrumTheme());
    ui->sa_theme_inherit->setChecked(appconf->getSpectrumTheme());
    lockslot = false;
}

void SettingsDlg::updateButtonStyle(bool white)
{
    ui->menu_edit->setIconStyle(white);
}

void SettingsDlg::setVisible(bool visible)
{
    refreshDevices();
    QDialog::setVisible(visible);
}

void SettingsDlg::updateInputSinks(){
    lockslot = true;
    ui->sa_input->clear();
    for ( const auto& dev: QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        ui->sa_input->addItem(dev.deviceName());

    QString qvSA(appconf->getSpectrumInput());
    int indexSA = ui->sa_input->findText(qvSA);
    if ( indexSA != -1 ) {
        ui->sa_input->setCurrentIndex(indexSA);
    }else{
        int index_fallback = ui->themeSelect->findText("default");
        if ( index_fallback != -1 )
            ui->sa_input->setCurrentIndex(index_fallback);
    }
    lockslot = false;
}

void SettingsDlg::showPECompatibilityScreen(){
    emit closeClicked();
    QTimer::singleShot(500,this,[this]{
        PulseeffectsCompatibility* wiz = new PulseeffectsCompatibility(appconf,m_mainwin);
        QHBoxLayout* lbLayout = new QHBoxLayout;
        QMessageOverlay* lightBox = new QMessageOverlay(m_mainwin);
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
        connect(wiz,&PulseeffectsCompatibility::wizardFinished,[=]{
            QPropertyAnimation *b = new QPropertyAnimation(eff,"opacity");
            b->setDuration(500);
            b->setStartValue(1);
            b->setEndValue(0);
            b->setEasingCurve(QEasingCurve::OutCirc);
            b->start(QPropertyAnimation::DeleteWhenStopped);
            connect(b,&QAbstractAnimation::finished, [=](){
                lightBox->hide();
            });
        });
    });
}
