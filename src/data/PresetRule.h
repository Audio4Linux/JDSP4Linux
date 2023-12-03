#ifndef PRESETRULE_H
#define PRESETRULE_H

#include <IOutputDevice.h>
#include <QDBusArgument>
#include <QJsonObject>

#include "model/RouteListModel.h"

class PresetRule
{
public:
    PresetRule(){}
    PresetRule(QJsonObject pkg)
    {
        deviceName = pkg.value("deviceName").toString();
        deviceId = pkg.value("deviceId").toString();
        preset = pkg.value("preset").toString();

        Route defaultRoute = RouteListModel::makeDefaultRoute();
        if(!pkg.contains("routeId") || !pkg.contains("routeName")) {
            routeName = QString::fromStdString(defaultRoute.description);
            routeId = QString::fromStdString(defaultRoute.name);
        }
        else {
            routeName = pkg.value("routeName").toString();
            routeId = pkg.value("routeId").toString();
        }
    }

    PresetRule(IOutputDevice device, Route route, const QString& _preset)
    {
        deviceName = QString::fromStdString(device.description);
        deviceId = QString::fromStdString(device.name);
        routeName = QString::fromStdString(route.description);
        routeId = QString::fromStdString(route.name);
        preset = _preset;
    }


    PresetRule(const QString& deviceId, const QString& routeId, const QString& preset)
        : deviceName(deviceId), deviceId(deviceId), preset(preset), routeName(routeId), routeId(routeId) {}

    PresetRule(const QString& deviceName, const QString& deviceId, const QString& routeName, const QString& routeId, const QString& preset)
        : deviceName(deviceName), deviceId(deviceId), preset(preset), routeName(routeName), routeId(routeId) {}

    QJsonObject toJson() const
    {
        QJsonObject pkg;
        pkg["deviceName"] = deviceName;
        pkg["deviceId"] = deviceId;
        pkg["preset"] = preset;
        pkg["routeName"] = routeName;
        pkg["routeId"] = routeId;
        return pkg;
    }

    bool operator==(const PresetRule &rhs)
    {
        return this->deviceName == rhs.deviceName &&
               this->deviceId == rhs.deviceId &&
               this->routeName == rhs.routeName &&
               this->routeId == rhs.routeId &&
               this->preset == rhs.preset;
    }

    QString deviceName;
    QString deviceId;
    QString preset;
    QString routeName;
    QString routeId;
};

Q_DECLARE_METATYPE(PresetRule)
Q_DECLARE_METATYPE(QList<PresetRule>)

inline QDBusArgument &operator<<(QDBusArgument &argument, const PresetRule &rule)
{
    argument.beginStructure();
    argument << rule.deviceName << rule.deviceId << rule.preset << rule.routeId;
    argument.endStructure();
    return argument;
}

// Retrieve the Position data from the D-Bus argument
inline const QDBusArgument &operator>>(const QDBusArgument &argument, PresetRule &rule)
{
    argument.beginStructure();
    argument >> rule.deviceName >> rule.deviceId >> rule.preset >> rule.routeId;
    argument.endStructure();
    return argument;
}

#endif // PRESETRULE_H
