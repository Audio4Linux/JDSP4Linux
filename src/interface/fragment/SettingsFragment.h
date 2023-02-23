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
#ifndef SETTINGS_H
#define SETTINGS_H

#include "config/AppConfig.h"
#include "BaseFragment.h"

class MainWindow;
class TrayIcon;
class IAudioService;
class PaletteEditor;

class QTreeWidgetItem;

namespace Ui
{
	class SettingsFragment;
}

class SettingsFragment :
    public BaseFragment
{
	Q_OBJECT

public:
    SettingsFragment(TrayIcon *trayIcon,
                     IAudioService *audioService,
	                 QWidget  *parent = nullptr);
	Ui::SettingsFragment *ui;
	~SettingsFragment();

	void refreshDevices();
	void refreshAll();
	void setVisible(bool visible) override;

public slots:
	void updateButtonStyle(bool white);

private slots:
    void onSavePathsClicked();
    void onExtractAssetsClicked();
    void onDefaultDeviceSelected();
    void onTreeItemSelected(QTreeWidgetItem *cur, QTreeWidgetItem* prev);
    void onAutoStartToggled();
    void onSystrayToggled();
    void onThemeSelected(int index);
    void onPaletteSelected(int index);
    void onBlocklistInvertToggled(bool state);
    void onBlocklistClearClicked();
    void onSetupWizardLaunchClicked();
    void onTrayEditorCommitted();
    void onTrayEditorReset();
    void onEqualizerHandlesToggled();
    void onLiveprogAutoExtractToggled();
    void onGithubLinkClicked();
    void onAeqDatabaseManageClicked();

signals:
	void launchSetupWizard();
	void reopenSettings();

private:
	TrayIcon *_trayIcon;
    IAudioService *_audioService;
    PaletteEditor *_paletteEditor;

    bool _lockslot = false;

};

#endif // SETTINGS_H
