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
#ifndef LOG_H
#define LOG_H

#include <string>
#include <QString>

class Log
{

public:
	enum LoggingMode
	{
		LM_ALL,
		LM_FILE,
		LM_STDOUT
	};

    enum Severity
    {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    static constexpr char* C_RESET = "\u001b[0m";
    static constexpr char* C_RED = "\u001b[31m";
    static constexpr char* C_YELLOW = "\u001b[33m";
    static constexpr char* C_WHITE = "\u001b[37m";
    static constexpr char* C_FAINTWHITE = "\u001b[2;37m";
    static constexpr char* C_BACKRED = "\u001b[41;37m";

	static void debug(const QString &log);
	static void information(const QString &log);
	static void warning(const QString &log);
    static void error(const QString &log);
    static void critical(const QString &log);
    static void write(const QString &log,
                      Severity       severity,
	                  LoggingMode    mode = LM_ALL);
	static void clear();

};

#endif // LOG_H