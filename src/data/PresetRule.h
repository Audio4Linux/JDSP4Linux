#ifndef PRESETRULE_H
#define PRESETRULE_H

#include <IOutputDevice.h>
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

    PresetRule(IOutputDevice device, QString _preset)
    {
        deviceName = QString::fromStdString(device.description);
        deviceId = QString::fromStdString(device.name);
        preset = _preset;;
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

#endif // PRESETRULE_H
