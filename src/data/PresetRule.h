#ifndef PRESETRULE_H
#define PRESETRULE_H

#include <IOutputDevice.h>
#include <QDBusArgument>
#include <QJsonObject>

class PresetRule
{
public:
    PresetRule(){}
    PresetRule(QJsonObject pkg)
    {
        deviceName = pkg.value("deviceName").toString();
        deviceId = pkg.value("deviceId").toString();
        preset = pkg.value("preset").toString();
    }

    PresetRule(QString _deviceId, QString _deviceName, QString _preset)
    {
        deviceName = _deviceName;
        deviceId = _deviceId;
        preset = _preset;
    }


    PresetRule(IOutputDevice device, QString _preset)
    {
        deviceName = QString::fromStdString(device.description);
        deviceId = QString::fromStdString(device.name);
        preset = _preset;
    }

    QJsonObject toJson() const
    {
        QJsonObject pkg;
        pkg["deviceName"] = deviceName;
        pkg["deviceId"] = deviceId;
        pkg["preset"] = preset;
        return pkg;
    }

    bool operator==(const PresetRule &rhs)
    {
        return this->deviceName == rhs.deviceName &&
               this->deviceId == rhs.deviceId &&
               this->preset == rhs.preset;
    }

    QString deviceName;
    QString deviceId;
    QString preset;
};

Q_DECLARE_METATYPE(PresetRule)
Q_DECLARE_METATYPE(QList<PresetRule>)

inline QDBusArgument &operator<<(QDBusArgument &argument, const PresetRule &rule)
{
    argument.beginStructure();
    argument << rule.deviceName << rule.deviceId << rule.preset;
    argument.endStructure();
    return argument;
}

// Retrieve the Position data from the D-Bus argument
inline const QDBusArgument &operator>>(const QDBusArgument &argument, PresetRule &rule)
{
    argument.beginStructure();
    argument >> rule.deviceName >> rule.deviceId >> rule.preset;
    argument.endStructure();
    return argument;
}

#endif // PRESETRULE_H
