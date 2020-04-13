#include "dbusproxy.h"
#include <QDebug>
#include <QDBusArgument>
#include "misc/loghelper.h"

DBusProxy::DBusProxy()
{
    QDBusConnection dbConnection = QDBusConnection::sessionBus();
    m_dbInterface = new QDBusInterface(
                "me.timschneeberger.JdspFx"
                , "/me/timschneeberger/JdspFx"
                , "me.timschneeberger.JdspFx"
                , dbConnection);

    if(!dbConnection.connect("me.timschneeberger.JdspFx",
                        "/me/timschneeberger/JdspFx",
                        "me.timschneeberger.JdspFx", "PropertiesCommitted",
                        this, SLOT(PropertiesSignalReceived(uint)))
            )
        LogHelper::writeLog("DBus signal handler not connected");
}

bool DBusProxy::isValid(){
    return m_dbInterface->isValid();
}

int DBusProxy::CommitProperties(PARAM_GROUP pg){
    if (m_dbInterface->isValid()) {
        QDBusMessage dbMessage = m_dbInterface->call("CommitPropertyGroups",QVariant(pg).toUInt());
        return dbMessage.arguments().at(0).toUInt();
    }
    return -1;
}

QString DBusProxy::GetVersion(){
    if (m_dbInterface->isValid()) {
        QDBusMessage dbMessage = m_dbInterface->call("GetVersion");
        return dbMessage.arguments().at(0).toString();
    }
    return "";
}

QString DBusProxy::GetGstVersion(){
    if (m_dbInterface->isValid()) {
        QDBusMessage dbMessage = m_dbInterface->call("GetGstVersion");
        return dbMessage.arguments().at(0).toString();
    }
    return "";
}

int DBusProxy::GetDriverStatus(PARAM_GET param){
    if (m_dbInterface->isValid()) {
        QDBusMessage dbMessage = m_dbInterface->call("GetDriverStatus",QVariant(param).toUInt());
        return dbMessage.arguments().at(0).toInt();
    }
    return -100;
}
bool DBusProxy::SubmitPropertyMap(const QVariantMap& map){
    if (m_dbInterface->isValid()) {
        for(const auto& e : map.keys()){
            m_dbInterface->setProperty(e.toUtf8().constData(),map.value(e));
        }
        return true;
    }
    return false;
}

QVariantMap DBusProxy::FetchPropertyMap(){
    if (m_dbInterface->isValid()) {
        QDBusMessage message = QDBusMessage::createMethodCall("me.timschneeberger.JdspFx", "/me/timschneeberger/JdspFx", QLatin1String("org.freedesktop.DBus.Properties"), QLatin1String("GetAll"));
        QList<QVariant> arguments;
        arguments << "me.timschneeberger.JdspFx";
        message.setArguments(arguments);
        QDBusConnection connection = QDBusConnection::sessionBus();
        QDBusMessage reply = connection.call(message);
        if(reply.arguments().empty())
            return QVariantMap();
        return qdbus_cast<QVariantMap>(*(static_cast<QDBusArgument*>((void*)reply.arguments().at(0).data())));
    }
    return QVariantMap();
}

bool DBusProxy::SetProperty(const QString& key, const QVariant& value){
    if (m_dbInterface->isValid()) {
        m_dbInterface->setProperty(key.toUtf8().constData(),value);
        return true;
    }
    return false;
}

QVariant DBusProxy::GetProperty(const QString& key){
    if (m_dbInterface->isValid())
        return m_dbInterface->property(key.toUtf8().constData());
    return QVariant();
}

void DBusProxy::PropertiesSignalReceived(uint bitmask){
    emit propertiesCommitted(bitmask);
}
