#ifndef IOUTPUTDEVICE_H
#define IOUTPUTDEVICE_H

#include <stdint.h>
#include <string>

#include <QDBusArgument>

class IOutputDevice
{

public:
    IOutputDevice(){}
    IOutputDevice(std::string name, std::string description) : name(name), description(description){}

    uint32_t id = ((uint32_t)0xffffffff);

    std::string name;

    std::string description;
};

Q_DECLARE_METATYPE(IOutputDevice)
Q_DECLARE_METATYPE(QList<IOutputDevice>)

inline QDBusArgument &operator<<(QDBusArgument &argument, const IOutputDevice &device)
{
    argument.beginStructure();
    argument << device.id << QString::fromStdString(device.name) << QString::fromStdString(device.description);
    argument.endStructure();
    return argument;
}

// Retrieve the Position data from the D-Bus argument
inline const QDBusArgument &operator>>(const QDBusArgument &argument, IOutputDevice &device)
{
    QString qName;
    QString qDesc;

    argument.beginStructure();
    argument >> device.id >> qName >> qDesc;
    argument.endStructure();

    device.name = qName.toStdString();
    device.description = qDesc.toStdString();
    return argument;
}

#endif // IOUTPUTDEVICE_H
