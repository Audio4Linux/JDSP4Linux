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
#ifndef APPCONFIGWRAPPER_H
#define APPCONFIGWRAPPER_H

#include <QObject>
#include "misc/stylehelper.h"
#include "container.h"
#include "io.h"
#include "misc/common.h"

using namespace std;

typedef enum class ReloadMethod{
    DIRECT_DBUS,
    USE_JDSPSCRIPT
}ReloadMethod;

class AppConfigWrapper : public QObject
{
    Q_OBJECT
public:
    AppConfigWrapper(StyleHelper* stylehelper);
    void saveAppConfig();
    void loadAppConfig();
    static QString getAppConfigFilePath();

    void setIrsPath(const QString& npath);
    QString getIrsPath();
    void setTheme(const QString&);
    QString getTheme();
    void setAutoFxMode(int);
    int getAutoFxMode();
    void setWhiteIcons(bool b);
    bool getWhiteIcons();
    void setCustompalette(const QString& s);
    QString getCustompalette();
    void setColorpalette(const QString&);
    QString getColorpalette();
    void setAutoFx(bool afx);
    bool getAutoFx();
    ReloadMethod getReloadMethod();
    void setReloadMethod(ReloadMethod mode);
    int getThememode();
    void setThememode(int mode);
    bool getGFix();
    void setGFix(bool);
    bool getMuteOnRestart();
    void setMuteOnRestart(bool on);
    QString getPath();
    void setPath(const QString& npath);
    QString getStylesheet();
    void setStylesheet(const QString&);
    void setTrayMode(int);
    int getTrayMode();
    void setSpectrumEnable(bool b);
    bool getSpetrumEnable();
    int getSpectrumBands();
    void setSpectrumBands(int number);
    int getSpectrumMinFreq();
    void setSpectrumMinFreq(int number);
    int getSpectrumMaxFreq();
    void setSpectrumMaxFreq(int number);
    void setSpectrumGrid(bool b);
    bool getSpetrumGrid();
    int getSpectrumTheme();
    void setSpectrumTheme(int number);
    void setSpectrumInput(const QString& npath);
    QString getSpectrumInput();
    int getSpectrumRefresh();
    void setSpectrumRefresh(int number);
    float getSpectrumMultiplier();
    void setSpectrumMultiplier(float number);
    void setEqualizerPermanentHandles(bool b);
    bool getEqualizerPermanentHandles();
    void setIntroShown(bool b);
    bool getIntroShown();
    QString getTrayContextMenu();
    void setTrayContextMenu(const QString &ctx);
    void setSpectrumShape(int number);
    int getSpectrumShape();
    bool getLegacyTabs();
    void setLegacyTabs(bool b);
    void setDDCPath(const QString &npath);
    QString getDDCPath();
    QString getLiveprogPath();
    void setLiveprogPath(const QString &npath);
    QString getGraphicEQConfigFilePath();
signals:
    void spectrumChanged();
    void spectrumReloadRequired();
    void styleChanged();
    void eqChanged();
private:
    ConfigContainer* appconf;
    StyleHelper*     m_stylehelper;
};

#endif // APPCONFIGWRAPPER_H
