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

class LogHelper
{
public:
    static void writeLog(const QString& log,int mode = 0);
    static void writeLogF(const QString& log,const QString& _path);
    static void clearLog();
};

#endif // LOGHELPER_H
