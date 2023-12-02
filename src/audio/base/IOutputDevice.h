#ifndef IOUTPUTDEVICE_H
#define IOUTPUTDEVICE_H

#include <stdint.h>
#include <string>

#include <QDBusArgument>

class Route
{
public:
    Route() {}
    Route(std::string name, std::string description) : name(name), description(description) {}

    std::string name;

    std::string description;
};

Q_DECLARE_METATYPE(Route)
Q_DECLARE_METATYPE(QList<Route>)

inline QDBusArgument &operator<<(QDBusArgument &argument, const Route &route)
{
    argument.beginStructure();
    argument << QString::fromStdString(route.name) << QString::fromStdString(route.description);
    argument.endStructure();
    return argument;
}

// Retrieve the Position data from the D-Bus argument
inline const QDBusArgument &operator>>(const QDBusArgument &argument, Route &route)
{
    QString qName;
    QString qDesc;

    argument.beginStructure();
    argument >> qName >> qDesc;
    argument.endStructure();

    route.name = qName.toStdString();
    route.description = qDesc.toStdString();
    return argument;
}

class IOutputDevice
{

public:
    IOutputDevice(){}
    IOutputDevice(std::string name, std::string description, std::string output_route_name, std::string output_route_description)
        : name(name), description(description), output_route_name(output_route_name), output_route_description(output_route_description){}

    uint32_t id = ((uint32_t)0xffffffff);

    std::string name;

    std::string description;

    std::string output_route_name;

    std::string output_route_description;

    QList<Route> output_routes;
};

Q_DECLARE_METATYPE(IOutputDevice)
Q_DECLARE_METATYPE(QList<IOutputDevice>)

inline QDBusArgument &operator<<(QDBusArgument &argument, const IOutputDevice &device)
{
    argument.beginStructure();
    argument << device.id << QString::fromStdString(device.name) << QString::fromStdString(device.description)
             << QString::fromStdString(device.output_route_name) << QString::fromStdString(device.output_route_description)
             << device.output_routes;
    argument.endStructure();
    return argument;
}

// Retrieve the Position data from the D-Bus argument
inline const QDBusArgument &operator>>(const QDBusArgument &argument, IOutputDevice &device)
{
    QString qName;
    QString qDesc;
    QString qOutputRoute;
    QString qOutputRouteDesc;

    argument.beginStructure();
    argument >> device.id >> qName >> qDesc >> qOutputRoute >> qOutputRouteDesc >> device.output_routes;
    argument.endStructure();

    device.name = qName.toStdString();
    device.description = qDesc.toStdString();
    device.output_route_name = qOutputRoute.toStdString();
    device.output_route_description = qOutputRouteDesc.toStdString();
    return argument;
}

#endif // IOUTPUTDEVICE_H
