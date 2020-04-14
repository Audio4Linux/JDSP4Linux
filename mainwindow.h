/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>

#include <misc/qjsontablemodel.h>

#include "visualization/audiostreamengine.h"
#include "visualization/spectrograph.h"

#include "config/io.h"
#include "config/container.h"
#include "dialog/settingsdlg.h"
#include "dialog/presetdlg.h"
#include "ui_settings.h"
#include "dialog/logdlg.h"
#include "misc/stylehelper.h"
#include "config/appconfigwrapper.h"
#include "misc/mathfunctions.h"
#include "misc/loghelper.h"
#include "misc/presetprovider.h"
#include "misc/common.h"
#include "config/dbusproxy.h"
#include "misc/overlaymsgproxy.h"

//Minimum required version of gst-plugin-jamesdsp
#define MINIMUM_PLUGIN_VERSION "2.0.0"

using namespace std;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum class Context;
public:
    Ui::MainWindow *ui;
    void LoadPresetFile(const QString&);
    void SavePresetFile(const QString&);
    AppConfigWrapper* getACWrapper();
    explicit MainWindow(QString exepath,bool statupInTray,bool allowMultipleInst,QWidget *parent = nullptr);
    void SetEQ(const QVector<float>& data);
    void SetIRS(const QString& irs,bool apply);
    void SetReverbData(PresetProvider::Reverb::sf_reverb_preset_data data);
    QString GetExecutablePath();
    void setVisible(bool visible) override;
    void setTrayVisible(bool visible);
    void LaunchFirstRunSetup();
    ~MainWindow();
    QMenu* buildAvailableActions();
    void updateTrayMenu(QMenu *menu);
    QMenu *buildDefaultActions();
    QMenu *getTrayContextMenu();
    void InitializeLegacyTabs();
    void ForceReload();

    SettingsDlg *settings_dlg;
    void setEQMode(int mode);
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
public slots:
    void RestartSpectrum();
    void Reset();
    void Restart();
    void raiseWindow();
    void ApplyConfig(bool restart=true);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
private slots:
    void DisableFX();
    void OnUpdate(bool = true);
    void OnRelease();
    void ResetEQ();
    void EqPresetSelectionUpdated();
    void StereoWidePresetSelectionUpdated();
    void BS2BPresetSelectionUpdated();
    void UpdateUnitLabel(int,QObject*alt=nullptr);
    void UpdateAllUnitLabels();
    void LoadExternalFile();
    void SaveExternalFile();
    void OpenLog();
    void DialogHandler();
    void updateTrayPresetList();
    void RefreshSpectrumParameters();
    void UpdateEqStringFromWidget();
    void UpdateBS2BStringFromWidget();
    void UpdateStereoWideStringFromWidget();
    void ReverbPresetSelectionUpdated();
    void reloadDDC();
    void reloadIRS();
    void reloadIRSFav();
    void reloadLiveprog();
    void restoreGraphicEQView();
    void saveGraphicEQView();
    void updateEQMode();
private:
    ConfigContainer* conf;
    AppConfigWrapper* m_appwrapper;
    StyleHelper* m_stylehelper;
    DBusProxy* m_dbus;
    QString m_exepath;

    bool m_startupInTraySwitch;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *quitAction;
    QAction *tray_disableAction;
    QMenu *tray_presetMenu;
    QMenu *tray_convMenu;

    QAction *spectrum;

    OverlayMsgProxy *msg_notrunning;
    OverlayMsgProxy *msg_launchfail;
    OverlayMsgProxy *msg_versionmismatch;

    PresetDlg *preset_dlg;
    LogDlg *log_dlg;

    QScopedPointer<QFrame> analysisLayout;
    Spectrograph* m_spectrograph;
    AudioStreamEngine* m_audioengine;

    bool m_irsNeedUpdate = false;
    bool settingsdlg_enabled=true;
    bool presetdlg_enabled=true;
    bool logdlg_enabled=true;
    bool lockapply = false;
    bool lockddcupdate = false;
    bool lockirsupdate = false;
    bool lockliveprogupdate = false;
    QString activeirs = "";
    QString activeddc = "";
    QString activeliveprog = "";

    QJsonTableModel* model;

    void InitializeSpectrum();
    void ToggleSpectrum(bool on,bool ctrl_visibility);
    void createTrayIcon();
    void UpdateTooltipLabelUnit(QObject* sender,const QString& text,bool);
    void LoadConfig(Context ctx = Context::Application);
    void ConnectActions();
    void ShowDBusError();
    void CheckDBusVersion();
    QVariantMap readConfig();
    void RunDiagnosticChecks();

    enum class Context{
        DBus,
        Application
    };
    void initGlobalTrayActions();
    void updateTrayConvolverList();
    void SetSpectrumVisibility(bool v);
    void SetReverbData(int osf, double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double p11, double p12, double p13, double p14, double p15, double p16);

    static void replaceTab(QTabWidget* tab, int index, QWidget *page, QString title = ""){
        if(title.isEmpty()) title = tab->tabText(index);
        auto toDelete = tab->widget(index);
        tab->removeTab(index);
        toDelete->deleteLater();
        tab->insertTab(index, page, title);
    }
    void reloadDDCDB();
    void setLiveprogSelection(QString path);
};

#endif // MAINWINDOW_H
