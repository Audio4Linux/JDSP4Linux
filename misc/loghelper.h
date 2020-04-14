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
#ifndef LOGHELPER_H
#define LOGHELPER_H
#include <QFile>
#include <QTime>
#include <QString>
#include <QDebug>

class LogHelper : QObject
{
    Q_OBJECT
public:

    enum LoggingMode{
        LM_ALL,
        LM_FILE,
        LM_STDOUT
    };

    static void debug(const QString &log);
    static void information(const QString& log);
    static void warning(const QString &log);
    static void error(const QString &log);
    static void write(const QString& log,LoggingMode mode = LM_ALL);
    static void clear();
};

#endif // LOGHELPER_H
