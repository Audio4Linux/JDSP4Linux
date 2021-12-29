#include "AppConfig.h"

#include "ConfigIO.h"
#include "utils/Common.h"

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

    DEFINE_KEY(AudioOutputUseDefault, true);
    DEFINE_KEY(AudioOutputDevice, "");
    DEFINE_KEY(AudioAppBlocklist, QStringList());
    DEFINE_KEY(AudioAppBlocklistInvert, false);

    DEFINE_KEY(AeqPlotDarkMode, false);

    DEFINE_KEY(ConvolverDefaultPath, ENCLOSE_QUOTES(getPath("irs")));
    DEFINE_KEY(VdcDefaultPath, ENCLOSE_QUOTES(getPath("vdc")));
    DEFINE_KEY(LiveprogDefaultPath, ENCLOSE_QUOTES(getPath("liveprog")));

    DEFINE_KEY(SendCrashReports, true);

    connect(this, &AppConfig::updated, this, &AppConfig::notify);

    load();
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

    Log::debug("AppConfig::isAppBlocked(\""+name+"\") -> " + ((invert ? !contains : contains) ? "true" : "false"));

    return invert ? !contains : contains;
}

QString AppConfig::getDspConfPath()
{
    return QString("%1/.config/jamesdsp/audio.conf").arg(QDir::homePath());
}

QString AppConfig::getPath(QString subdir)
{
    return QString("%1/.config/jamesdsp/%2").arg(QDir::homePath()).arg(subdir);
}

QString AppConfig::getCachePath(QString subdir)
{
    return QString("%1/.cache/jamesdsp/%2").arg(QDir::homePath()).arg(subdir);
}

void AppConfig::save()
{
    auto file = QString("%1/.config/jamesdsp/application.conf").arg(QDir::homePath());
    ConfigIO::writeFile(file, _appconf->getConfigMap());
}

void AppConfig::load()
{
    auto map = ConfigIO::readFile(QString("%1/.config/jamesdsp/application.conf").arg(QDir::homePath()));
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
