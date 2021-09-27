#include "AppConfig.h"

#include "ConfigIO.h"
#include "utils/Common.h"

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

    DEFINE_KEY(SpectrumEnabled, false);
    DEFINE_KEY(SpectrumGrid, false);
    DEFINE_KEY(SpectrumBands, 0);
    DEFINE_KEY(SpectrumMinFreq, 0);
    DEFINE_KEY(SpectrumMaxFreq, 0);
    DEFINE_KEY(SpectrumTheme, 0);
    DEFINE_KEY(SpectrumRefresh, 0);
    DEFINE_KEY(SpectrumMultiplier, 0);

    DEFINE_KEY(EqualizerShowHandles, false);

    DEFINE_KEY(SetupDone, false);
    DEFINE_KEY(ExecutablePath, "");
    DEFINE_KEY(VdcLastDatabaseId, -1);

    DEFINE_KEY(AudioOutputUseDefault, true);
    DEFINE_KEY(AudioOutputDevice, "");
    DEFINE_KEY(AudioAppBlocklist, QStringList());

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

QString AppConfig::getDspConfPath()
{
    return QString("%1/.config/jamesdsp/audio.conf").arg(QDir::homePath());
}

QString AppConfig::getPath(QString subdir)
{
    return QString("%1/.config/jamesdsp/%2").arg(QDir::homePath()).arg(subdir);
}

void AppConfig::setIrsPath(const QString &npath)
{
    _appconf->setValue("ConvolverDefaultPath", QVariant(QString("\"%1\"").arg(npath)));
    save();
}

QString AppConfig::getIrsPath()
{
    QString irs_path = chopFirstLastChar(_appconf->getString("ConvolverDefaultPath", false));

    if (irs_path.length() < 2)
    {
        return QString("%1/IRS").arg(QDir::homePath());
    }

    return irs_path;
}

void AppConfig::setDDCPath(const QString &npath)
{
    _appconf->setValue("VdcDefaultPath", QVariant(QString("\"%1\"").arg(npath)));
    save();
}

QString AppConfig::getDDCPath()
{
    QString irs_path = chopFirstLastChar(_appconf->getString("VdcDefaultPath", false));

    if (irs_path.length() < 2)
    {
        return QString("%1/DDC").arg(QDir::homePath());
    }

    return irs_path;
}

void AppConfig::setLiveprogPath(const QString &npath)
{
    _appconf->setValue("LiveprogDefaultPath", QVariant(QString("\"%1\"").arg(npath)));
    save();
}

QString AppConfig::getLiveprogPath()
{
    QString absolute = QFileInfo(getDspConfPath()).absoluteDir().absolutePath();
    QString lp_path  = chopFirstLastChar(_appconf->getString("LiveprogDefaultPath", false));

    if (lp_path.length() < 2)
    {
        QDir(absolute).mkdir("liveprog");
        QString defaultstr = QString("%1/liveprog").arg(absolute);
        setLiveprogPath(defaultstr);
        return defaultstr;
    }

    return lp_path;
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

QString AppConfig::getGraphicEQConfigFilePath()
{
    return pathAppend(QFileInfo(getDspConfPath()).absoluteDir().absolutePath(), "graphiceq.conf");
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
    case SpectrumEnabled:
    case SpectrumBands:
    case SpectrumGrid:
    case SpectrumMinFreq:
    case SpectrumMaxFreq:
    case SpectrumMultiplier:
    case SpectrumTheme:
        emit spectrumChanged(false);
        break;
    case SpectrumRefresh:
        emit spectrumChanged(true);
        break;
    default:
        break;
    }
}
