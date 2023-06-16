#include "AppConfig.h"

#include "ConfigIO.h"
#include "utils/AutoStartManager.h"
#include "utils/Common.h"

#include <QStandardPaths>

#define ENCLOSE_QUOTES(x) "\"" + x + "\""

#define DEFINE_KEY(name, defaultValue) \
    definitions[name] = QVariant(defaultValue);

AppConfig::AppConfig()
{
    _appconf = new ConfigContainer();

    DEFINE_KEY(LiveprogAutoExtract, true);

    DEFINE_KEY(Theme, "Fusion");
    DEFINE_KEY(ThemeColors, "Default");
    DEFINE_KEY(ThemeColorsCustom, "");
    DEFINE_KEY(ThemeColorsCustomWhiteIcons, false);

    DEFINE_KEY(TrayIconEnabled, true);
    DEFINE_KEY(TrayIconMenu, "");

    DEFINE_KEY(EqualizerShowHandles, false);

    DEFINE_KEY(SetupDone, false);
    DEFINE_KEY(ExecutablePath, "");
    DEFINE_KEY(VdcLastDatabaseId, -1);
    DEFINE_KEY(LastWindowGeometry, QByteArray());
#ifdef USE_PORTALS
    DEFINE_KEY(AutoStartEnabled, false);
#else
    DEFINE_KEY(AutoStartEnabled, QFile(AutostartManager::getAutostartPath()).exists());
#endif

    DEFINE_KEY(AudioOutputUseDefault, true);
    DEFINE_KEY(AudioOutputDevice, "");
    DEFINE_KEY(AudioAppBlocklist, QStringList());
    DEFINE_KEY(AudioAppBlocklistInvert, false);
    DEFINE_KEY(AudioInactivityTimeout, 10);

    DEFINE_KEY(AeqPlotDarkMode, false);

    DEFINE_KEY(ConvolverDefaultPath, ENCLOSE_QUOTES(getPath("irs")));
    DEFINE_KEY(VdcDefaultPath, ENCLOSE_QUOTES(getPath("vdc")));
    DEFINE_KEY(LiveprogDefaultPath, ENCLOSE_QUOTES(getPath("liveprog")));

    connect(this, &AppConfig::updated, this, &AppConfig::notify);

    load();
}

void AppConfig::setBytes(const Key &key, const QByteArray &value)
{
    set(key, QVariant(QString::fromLocal8Bit(value.toBase64())));
}

void AppConfig::set(const Key &key, const QStringList &value)
{
    set(key, value.join(';'));
}

void AppConfig::set(const Key &key, const QVariant &value)
{
    _appconf->setValue(QVariant::fromValue(key).toString(), value);
    emit updated(key, value);
    save();
}

bool AppConfig::exists(const Key &key)
{
    bool exists;
    auto skey = QVariant::fromValue(key).toString();
    auto variant = _appconf->getVariant(skey, true, &exists);
    return exists;
}

bool AppConfig::isAppBlocked(const QString &name) const
{
    const auto& blocklist = get<QStringList>(AppConfig::AudioAppBlocklist);
    bool invert = get<bool>(AppConfig::AudioAppBlocklistInvert);
    bool contains = blocklist.contains(name);

    Log::debug(name + " is " + ((invert ? !contains : contains) ? "blocked" : "not blocked"));

    return invert ? !contains : contains;
}

QString AppConfig::getDspConfPath()
{
    return getPath("audio.conf");
}

QString AppConfig::getPath(QString subdir)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if(path.isEmpty())
    {
        path = QString("%1/.config/").arg(QDir::homePath());
    }

    return QString("%1/jamesdsp/%2").arg(path).arg(subdir);
}

QString AppConfig::getCachePath(QString subdir)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if(path.isEmpty())
    {
        path = QString("%1/.cache/").arg(QDir::homePath());
    }
    return QString("%1/jamesdsp/%2").arg(path).arg(subdir);
}

void AppConfig::save()
{
    auto file = getPath("application.conf");
    ConfigIO::writeFile(file, _appconf->getConfigMap());
}

void AppConfig::load()
{
    auto map = ConfigIO::readFile(getPath("application.conf"));
    _appconf->setConfigMap(map);

    for(const auto& key : map.keys())
    {
        auto ekey  = static_cast<Key>(QMetaEnum::fromType<Key>().keyToValue(key.toLocal8Bit().constData()));
        emit updated(ekey, map[key]);
    }
}

QString AppConfig::getGraphicEqStatePath()
{
    return getPath("graphiceq.conf");
}

void AppConfig::notify(const Key &key, const QVariant &value)
{
    switch(key)
    {
    case Theme:
    case ThemeColors:
    case ThemeColorsCustom:
    case ThemeColorsCustomWhiteIcons:
        emit themeChanged(key, value);
        break;
    default:
        break;
    }
}
