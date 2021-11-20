#include "PresetManager.h"

#include "config/AppConfig.h"
#include "config/DspConfig.h"

#include <QFile>

PresetManager::PresetManager()
{

}

void PresetManager::load(const QString &filename)
{
    const QString &src  = filename;
    const QString  dest = AppConfig::instance().getDspConfPath();

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    Log::debug("MainWindow::loadPresetFile: Loading from " + filename);
    DspConfig::instance().load();
}

void PresetManager::save(const QString &filename)
{
    const QString  src  = AppConfig::instance().getDspConfPath();
    const QString &dest = filename;

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    Log::debug("MainWindow::savePresetFile: Saving to " + filename);
}
