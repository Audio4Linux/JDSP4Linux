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
#include "utils/Common.h"
#include "utils/Log.h"

#include <QObject>
#include <QMetaEnum>
#include <optional>

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

        EqualizerShowHandles,

        SetupDone,
        ExecutablePath,
        VdcLastDatabaseId,
        LastWindowGeometry,

        AudioOutputUseDefault,
        AudioOutputDevice,
        AudioAppBlocklist,
        AudioAppBlocklistInvert,
        AudioInactivityTimeout,

        AeqPlotDarkMode,

        ConvolverDefaultPath,
        VdcDefaultPath,
        LiveprogDefaultPath
    };
    Q_ENUM(Key);

    void setBytes(const Key &key,
             const QByteArray &value);

    void set(const Key &key,
             const QStringList &value);

    void set(const Key &key,
             const QVariant &value);

    template<class T>
    static T convertVariant(QVariant variant){
        if constexpr (std::is_same_v<T, QVariant>) {
            return variant;
        }
        if constexpr (std::is_same_v<T, QByteArray>) {
            return QByteArray::fromBase64(variant.toString().toLocal8Bit());
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

        Log::error("Unknown generic type T");
    }

    template<class T>
    T get(const Key &key) const
    {
        bool exists;
        auto skey = QVariant::fromValue(key).toString();
        auto variant = _appconf->getVariant(skey, true, &exists);

        if(!exists && definitions.contains(key))
        {
            return convertVariant<T>(definitions[key]);
        }

        return convertVariant<T>(variant);
    }

    bool exists(const Key &key);

    bool isAppBlocked(const QString& name) const;

#define DEFINE_USER_PATH(name,key) \
    QString get##name##Path(QString subdir = "") const \
    { \
        QString current = chopFirstLastChar(get<QString>(key)); \
        if (!QDir(current).exists()) \
        { \
            QDir().mkpath(current); \
        } \
        if(!subdir.isEmpty()) \
        { \
            return QString("%1/%2").arg(current).arg(subdir); \
        } \
        return current; \
    } \
    void set##name##Path(const QString& newVal) \
    { \
        set(key, QVariant(QString("\"%1\"").arg(newVal))); \
    }

DEFINE_USER_PATH(Irs, ConvolverDefaultPath)
DEFINE_USER_PATH(Vdc, VdcDefaultPath)
DEFINE_USER_PATH(Liveprog, LiveprogDefaultPath)

#undef DEFINE_USER_PATH

    QString getPath(QString subdir = "");
    QString getCachePath(QString subdir);
    QString getDspConfPath();
    QString getGraphicEqStatePath();

    void save();
    void load();

private slots:
    void notify(const Key& key, const QVariant& value);

signals:
    void themeChanged(const Key&, const QVariant&);
    void updated(const Key&, const QVariant&);

private:
    QMap<Key, QVariant> definitions;
	ConfigContainer *_appconf;
};

#endif // APPCONFIGWRAPPER_H
