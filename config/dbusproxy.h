/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */
#ifndef DBUSPROXY_H
#define DBUSPROXY_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>

class DBusProxy : public QObject
{
    Q_OBJECT
public:
    typedef enum
    {
        PARAM_GROUP_NONE    = 0x0000,
        PARAM_GROUP_AGC     = (1u << 0),
        PARAM_GROUP_AX      = (1u << 1),
        PARAM_GROUP_COLM    = (1u << 2),
        PARAM_GROUP_CONV    = (1u << 3),
        PARAM_GROUP_CURE    = (1u << 4),
        PARAM_GROUP_DS      = (1u << 5),
        PARAM_GROUP_DYNSYS  = (1u << 6),
        PARAM_GROUP_EQ      = (1u << 7),
        PARAM_GROUP_FETCOMP = (1u << 8),
        PARAM_GROUP_LIM     = (1u << 9),
        PARAM_GROUP_MSWITCH = (1u << 10),
        PARAM_GROUP_REVERB  = (1u << 11),
        PARAM_GROUP_TUBE    = (1u << 12),
        PARAM_GROUP_VB      = (1u << 13),
        PARAM_GROUP_VC      = (1u << 14),
        PARAM_GROUP_VHE     = (1u << 15),
        PARAM_GROUP_VSE     = (1u << 16),
        PARAM_GROUP_ALL     = 0x1FFFF
    } PARAM_GROUP;
    typedef enum
    {
        PARAM_GET_STATUS_BEGIN = 0x08000,
        PARAM_GET_DRIVER_VERSION,
        PARAM_GET_NEONENABLED,
        PARAM_GET_ENABLED,
        PARAM_GET_DRVCANWORK,
        PARAM_GET_EFFECT_TYPE,
        PARAM_GET_SAMPLINGRATE,
        PARAM_GET_CONVKNLID,
        PARAM_GET_STATUS_END
    } PARAM_GET;
    static constexpr int E_INVALID_INTF = -100;

    DBusProxy();
    bool isValid();
    int CommitProperties(PARAM_GROUP pg);
    QString GetVersion();
    QString GetGstVersion();
    int GetDriverStatus(PARAM_GET param);
    QVariantMap FetchPropertyMap();
    bool SubmitPropertyMap(const QVariantMap& map);
    bool SetProperty(const QString& key, const QVariant& value);
    QVariant GetProperty(const QString& key);
private slots:
    void PropertiesSignalReceived(uint);
signals:
    void propertiesCommitted(uint);
private:
    QDBusInterface *m_dbInterface;
};

#endif // DBUSPROXY_H
