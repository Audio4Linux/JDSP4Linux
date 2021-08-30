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
#include <QMainWindow>
#include <QSystemTrayIcon>

#include "data/PresetProvider.h"

// Minimum required version of gst-plugin-jamesdsp
#define MINIMUM_PLUGIN_VERSION "2.0.0"

class AppConfig;
class EELParser;
class ConfigContainer;
class StyleHelper;
class DBusProxy;
class OverlayMsgProxy;
class EELEditor;
class PresetDialog;
class SettingsFragment;
class AudioStreamEngine;
class Spectrograph;
class QJsonTableModel;
class TrayIcon;
class QVBoxLayout;
class AudioManager;

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

	static void loadPresetFile(const QString &);
	static void savePresetFile(const QString &);
	QString     executablePath();
	void        setEq(const QVector<double> &data);
	void        setReverbData(PresetProvider::Reverb::sf_reverb_preset_data data);
	void        setEqMode(int mode);
	void        updateDDCSelection();

    Ui::MainWindow *ui;
    SettingsFragment *settings_dlg;

protected:
	void        closeEvent(QCloseEvent *event) override;
	void        setVisible(bool visible) override;

public slots:
	void        restartSpectrum();
	void        reset();
	void        restart();
	void        raiseWindow();
	void        applyConfig();
	void        trayIconActivated();
	int         extractDefaultEelScripts(bool allowOverride,
	                                     bool user);
	void        launchFirstRunSetup();

private slots:
	void        disableFx();
	void        onUpdate(bool = true);
	void        onRelease();
	void        resetEQ();
	void        eqPresetSelectionUpdated();
	void        updateUnitLabel(int,
	                            QObject *alt = nullptr);
	void        updateAllUnitLabels();
	void        loadExternalFile();
	void        saveExternalFile();
	void        dialogHandler();
	void        refreshSpectrumParameters();
	void        updateEqStringFromWidget();
	void        bs2bPresetSelectionUpdated();
	void        reverbPresetSelectionUpdated();
	void        reloadDDC();
	void        reloadIRS();
	void        reloadIRSFav();
	void        reloadLiveprog();
	void        restoreGraphicEQView();
	void        saveGraphicEQView();
	void        updateEQMode();
	void        fireTimerSignal();
	void        resetLiveprogParams();
	void        updateFromEelEditor(QString path);
	void        convolverWaveformEdit();

private:
	StyleHelper *m_stylehelper;
	QTimer *refreshTick;

	bool m_startupInTraySwitch;
	TrayIcon *trayIcon;

	QAction *spectrum;
	EELParser *_eelparser;

	PresetDialog *preset_dlg;
	EELEditor *m_eelEditor;

	QScopedPointer<QFrame> analysisLayout;
	Spectrograph *m_spectrograph;
	AudioStreamEngine *m_audioengine;

	QFrame *settingsFragmentHost;
	QVBoxLayout *settingsHostLayout;

    AudioManager* audioManager;

	bool spectrumReloadSignalQueued   = false;
	bool lockapply                    = false;
	bool lockddcupdate                = false;
	bool lockirsupdate                = false;
	bool lockliveprogupdate           = false;

	QString activeirs                 = "";
	QString activeddc                 = "";
	QString activeliveprog            = "";
	QString irAdvancedWaveformEditing = "";

	QJsonTableModel *model;

	void initializeSpectrum();
	void toggleSpectrum(bool on,
	                    bool ctrl_visibility);
	void updateTooltipLabelUnit(QObject       *sender,
	                            const QString &text,
	                            bool);
	void loadConfig();
	void connectActions();

	void setSpectrumVisibility(bool v);
	void reloadDDCDB();
	void setLiveprogSelection(QString path);

	static void replaceTab(QTabWidget *tab,
	                       int         index,
	                       QWidget    *page,
	                       QString     title = "")
	{
		if (title.isEmpty())
		{
			title = tab->tabText(index);
		}

		auto toDelete = tab->widget(index);
		tab->removeTab(index);
		toDelete->deleteLater();
		tab->insertTab(index, page, title);
	}

	void updateIrsSelection();

};

#endif // MAINWINDOW_H
