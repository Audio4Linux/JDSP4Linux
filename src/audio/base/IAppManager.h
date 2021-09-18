#ifndef IAPPMANAGER_H
#define IAPPMANAGER_H

#include "AppNode.h"
#include <QObject>

class IAppManager : public QObject
{
    Q_OBJECT
public:
    virtual ~IAppManager() {}

    virtual QList<AppNode> activeApps() const = 0;

signals:
    void appAdded(const AppNode& node);
    void appChanged(const AppNode& node);
    void appRemoved(const uint id);

};

#endif // IAPPMANAGER_H
