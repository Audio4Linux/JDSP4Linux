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

#include <QCloseEvent>
#include <QFrame>
#include <QItemSelection>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include "interface/fragment/AppManagerFragment.h"
#include "interface/fragment/FragmentHost.h"
#include "data/PresetProvider.h"

class IAudioService;
class AppConfig;
class EELParser;
class ConfigContainer;
class StyleHelper;
class DBusProxy;
class OverlayMsgProxy;
class EELEditor;
class PresetFragment;
class SettingsFragment;
class AudioStreamEngine;
class Spectrograph;
class QJsonTableModel;
class TrayIcon;
class QVBoxLayout;
class AudioManager;
class SingleInstanceMonitor;
class StatusFragment;

using namespace std;
namespace Ui
{
	class MainWindow;
}

class MainWindow :
	public QMainWindow
{
	Q_OBJECT
	enum class Context;

public:
	explicit MainWindow(QString  exepath,
	                    bool     statupInTray,
                        bool     allowMultipleInst,
	                    QWidget *parent = nullptr);
	~MainWindow();

protected:
    void        resizeEvent(QResizeEvent* event) override;
    void        closeEvent(QCloseEvent *event) override;

public slots:
    void        onResetRequested();
    void        onRelinkRequested();
	void        raiseWindow();
	int         extractDefaultEelScripts(bool allowOverride,
	                                     bool user);
	void        launchFirstRunSetup();

private slots:
    void        applyConfig();

    void        onPassthroughToggled();
    void        onFragmentRequested();

	void        resetEQ();


    void        updateAllUnitLabels();
    void        updateUnitLabel(int,
	                            QObject *alt = nullptr);


	void        loadExternalFile();
	void        saveExternalFile();

    void        onBs2bPresetUpdated();
    void        onReverbPresetUpdated();
    void        onEqPresetUpdated();
    void        onEqModeUpdated();

	void        restoreGraphicEQView();
	void        saveGraphicEQView();

    void        fireTimerSignal();
    void        onConvolverWaveformEdit();

    void        onTrayIconActivated();

    void        setVdcFile(const QString &path);
    void        setIrsFile(const QString &path);

    void        determineEqPresetName();

    void        onVdcDatabaseSelected(const QItemSelection &, const QItemSelection &);
    void        onAutoEqImportRequested();
private:
    Ui::MainWindow *ui;

    StyleHelper *_styleHelper;
    QTimer *_refreshTick;

    bool _startupInTraySwitch;
    TrayIcon *_trayIcon;

    EELEditor *_eelEditor;

    SingleInstanceMonitor* _singleInstance;

    FragmentHost<AppManagerFragment*>* _appMgrFragment = nullptr;
    FragmentHost<StatusFragment*>* _statusFragment     = nullptr;
    FragmentHost<SettingsFragment*>* _settingsFragment = nullptr;
    FragmentHost<PresetFragment*>* _presetFragment     = nullptr;

    QAction *_spectrum;
    QScopedPointer<QFrame> _spectrumLayout;
    Spectrograph *_spectrumWidget;
    AudioStreamEngine *_spectrumEngine;

    IAudioService* _audioService       = nullptr;

    bool _spectrumReloadSignalQueued   = false;
    bool _blockApply                   = false;
    bool _lockliveprogupdate           = false;

    QString _currentImpuleResponse    = "";
    QString _currentVdc               = "";
    QString _currentConvWaveformEdit  = "";

    void updateTooltipLabelUnit(QObject       *sender,
	                            const QString &text,
	                            bool);
	void loadConfig();
	void connectActions();

    void determineIrsSelection();
    void determineVdcSelection();

    void setEq(const QVector<double> &data);
    void setEqMode(int mode);
    void setReverbData(PresetProvider::Reverb::sf_reverb_preset_data data);


};

#endif // MAINWINDOW_H
