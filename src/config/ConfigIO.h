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
#ifndef CONFIGIO_H
#define CONFIGIO_H

#include <QObject>
#include <QVariantMap>

class ConfigIO
{
public:
	static QString     writeString(const QVariantMap &map);
	static void        writeFile(const QString &    path,
	                             const QVariantMap &map,
	                             const QString &    prefix = "");
	static QVariantMap readFile(const QString &path);
	static QVariantMap readString(const QString &string);
	static bool        readLine(const QString &line,
	                            QPair<QString, QVariant> &out);

};

#endif // CONFIGIO_H
