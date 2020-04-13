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

#include <QDialog>
#include "ui_settings.h"
#include "config/appconfigwrapper.h"

class MainWindow;

class SettingsDlg : public QDialog
{
    Q_OBJECT

public:
    SettingsDlg(MainWindow* mainwin, QWidget *parent = nullptr);
    Ui::settings *ui;
    ~SettingsDlg();
    void updateInputSinks();
    void refreshDevices();
    void setVisible(bool visible) override;
    void refreshAll();
public slots:
    void updateButtonStyle(bool white);
signals:
        void closeClicked();
private:
        AppConfigWrapper* appconf;
        MainWindow* m_mainwin;
        void showPECompatibilityScreen();
};

#endif // SETTINGS_H
