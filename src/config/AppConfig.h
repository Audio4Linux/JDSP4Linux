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

#include "ConfigContainer.h"
#include "utils/Log.h"

#include <QObject>
#include <QMetaEnum>

using namespace std;

class AppConfig :
	public QObject
{
	Q_OBJECT

public:
	static AppConfig &instance()
	{
		static AppConfig _instance;
		return _instance;
	}

	AppConfig(AppConfig const &)       = delete;
	void operator= (AppConfig const &) = delete;

    AppConfig();

    enum Key {
        LiveprogAutoExtract,

        Theme,
        ThemeColors,
        ThemeColorsCustom,
        ThemeColorsCustomWhiteIcons,

        TrayIconEnabled,
        TrayIconMenu,

        SpectrumEnabled,
        SpectrumGrid,
        SpectrumBands,
        SpectrumMinFreq,
        SpectrumMaxFreq,
        SpectrumTheme,
        SpectrumRefresh,
        SpectrumMultiplier,

        EqualizerShowHandles,

        SetupDone,
        ExecutablePath,
        VdcLastDatabaseId,

        AudioOutputUseDefault,
        AudioOutputDevice,
        AudioAppBlocklist,
        AudioAppBlocklistInvert,

        AeqPlotDarkMode
    };
    Q_ENUM(Key);

    void set(const Key &key,
             const QStringList &value);

    void set(const Key &key,
             const QVariant &value);

    template<class T>
    static T convertVariant(QVariant variant){
        if constexpr (std::is_same_v<T, QVariant>) {
            return variant;
        }
        if constexpr (std::is_same_v<T, std::string>) {
            return variant.toString().toStdString();
        }
        if constexpr (std::is_same_v<T, QString>) {
            return variant.toString();
        }
        if constexpr (std::is_same_v<T, QStringList>) {
            return variant.toString().split(';');
        }
        if constexpr (std::is_same_v<T, int>) {
            return variant.toInt();
        }
        if constexpr (std::is_same_v<T, float>) {
            return variant.toFloat();
        }
        if constexpr (std::is_same_v<T, bool>) {
            return variant.toBool();
        }

        Log::error("AppConfig::convertVariant<T>: Unknown type T");
    }

    template<class T>
    T get(const Key &key) const
    {
        bool exists;
        auto skey = QVariant::fromValue(key).toString();
        auto variant = _appconf->getVariant(skey, true, &exists);

        QVariant defaultValue = definitions[key];

        if(!exists)
        {
            return convertVariant<T>(defaultValue);
        }

        return convertVariant<T>(variant);
    }

    bool isAppBlocked(const QString& name) const;

    QString getDspConfPath();

    QString getPath(QString subdir = "");

    void setIrsPath(const QString &npath);

    QString getIrsPath();

    void setDDCPath(const QString &npath);

    QString getDDCPath();

    void setLiveprogPath(const QString &npath);

    QString getLiveprogPath();

    void save();

    void load();

    QString getGraphicEQConfigFilePath();

    QString getCachePath(QString subdir);
private slots:
    void notify(const Key& key, const QVariant& value);

signals:
    void spectrumChanged(bool needReload);
    void themeChanged(const Key&, const QVariant&);
    void updated(const Key&, const QVariant&);

private:
    QMap<Key, QVariant> definitions;
	ConfigContainer *_appconf;
};

#endif // APPCONFIGWRAPPER_H
