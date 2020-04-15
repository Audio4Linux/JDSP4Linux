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
#ifndef CONFIGCONTAINER_H
#define CONFIGCONTAINER_H

#include <QObject>
#include <QVariant>

class ConfigContainer
{
public:
    ConfigContainer();
    void setValue(const QString& key,QVariant value);
    QVariant getVariant(const QString& key, bool silent=false);
    QString getString(const QString& key,bool setToDefaultIfMissing=true);
    int getInt(const QString& key, bool setToDefaultIfMissing=true, int defaultVal = 0);
    float getFloat(const QString& key);
    bool getBool(const QString& key, bool setToDefaultIfMissing=true, bool defaultVal = false);

    QVariantMap getConfigMap();
    void setConfigMap(const QVariantMap& newmap);

private:
    QVariantMap map = QVariantMap();

};

#endif // CONFIGCONTAINER_H
