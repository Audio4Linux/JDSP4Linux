#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QObject>

class AssetManager : public QObject
{
    Q_OBJECT
public:
    static AssetManager &instance()
    {
        static AssetManager _instance;
        return _instance;
    }

    enum AssetType
    {
        AT_IR,
        AT_VDC,
        AT_LIVEPROG
    };

    AssetManager(AssetManager const &) = delete;
    AssetManager(QObject* parent = nullptr);

    int extractGroup(AssetType type, bool allowOverride);

public slots:
    int extractAll(bool force = false);

};

#endif // ASSETMANAGER_H
