#include "AutoStartManager.h"
#include "config/AppConfig.h"
#include "config/ConfigContainer.h"
#include "config/ConfigIO.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#define DESKTOP_FILE "jdsp-gui.desktop"

AutostartManager::AutostartManager()
{}

void AutostartManager::setEnabled(bool enabled)
{
    if(!enabled) {
        QFile::remove(DESKTOP_FILE);
        return;
    }

	ConfigContainer *conf = new ConfigContainer();
    conf->setValue("Exec", QString("%0 --tray").arg(AppConfig::instance().get<QString>(AppConfig::ExecutablePath)));
    conf->setValue("Name",                      "JamesDSP for Linux (Systray)");
	conf->setValue("StartupNotify",             false);
	conf->setValue("Terminal",                  false);
	conf->setValue("Type",                      "Application");
    conf->setValue("Version",                   "1.0");
    conf->setValue("X-GNOME-Autostart-Delay",   6);
	conf->setValue("X-GNOME-Autostart-enabled", true);
	conf->setValue("X-KDE-autostart-after",     "panel");
	conf->setValue("X-KDE-autostart-phase",     2);
    conf->setValue("X-MATE-Autostart-Delay",    6);
    ConfigIO::writeFile(getAutostartPath(DESKTOP_FILE), conf->getConfigMap(), "[Desktop Entry]");
}

bool AutostartManager::isEnabled()
{
    auto path = getAutostartPath(DESKTOP_FILE);
	ConfigContainer conf;
	conf.setConfigMap(ConfigIO::readFile(path));
    return QFile::exists(path);
}

QString AutostartManager::getAutostartPath(const QString &filename)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if(path.isEmpty())
    {
        path = QString("%1/.config/").arg(QDir::homePath());
    }

    return QString("%1/autostart/%2").arg(path, filename);
}
