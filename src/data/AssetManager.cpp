#include "AssetManager.h"

#include "config/AppConfig.h"

#include <QDirIterator>
#include <QTextStream>

AssetManager::AssetManager(QObject *parent)
    : QObject{parent}
{

}

int AssetManager::extractGroup(AssetType type, bool allowOverride)
{
    QString internalPath;
    QString externalPath;
    switch(type)
    {
    case AssetManager::AT_IR:
        internalPath = ":/assets/irs";
        externalPath = AppConfig::instance().getIrsPath();
        break;
    case AssetManager::AT_VDC:
        internalPath = ":/assets/vdc";
        externalPath = AppConfig::instance().getVdcPath();
        break;
    case AssetManager::AT_LIVEPROG:
        internalPath = ":/assets/liveprog";
        externalPath = AppConfig::instance().getLiveprogPath();
        break;
    }

    if(!QDir(externalPath).exists())
    {
        QDir().mkpath(externalPath);
    }

    QDirIterator it(internalPath, QDirIterator::NoIteratorFlags);
    int i = 0;
    while (it.hasNext())
    {
        QFile sourceFile(it.next());
        QFile targetFile(externalPath + QDir::separator() + QFileInfo(sourceFile).fileName());

        if (targetFile.exists() && !allowOverride)
        {
            continue;
        }
        else if(targetFile.exists())
        {
            targetFile.remove();
        }

        sourceFile.copy(targetFile.fileName());
        targetFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        i++;
    }

    if (i > 0)
    {
        Log::debug(QString("%1 assets extracted (type %2)").arg(i).arg(type));
    }

    return i;
}

int AssetManager::extractAll(bool force)
{
    int i = 0;
    i += extractGroup(AT_IR, true);
    i += extractGroup(AT_VDC, true);
    i += extractGroup(AT_LIVEPROG, force ? true : false);
    return i;
}

