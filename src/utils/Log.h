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
#include <QTextStream>

// TODO Move Utils.h out of AudioDrivers/Base into a common subproject
#include "Utils.h"

class Log
{

private:
    Log(){};

public:
	enum LoggingMode
	{
        LM_UNSPECIFIED,
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

    Log(const Log&) = delete;
    Log& operator=(const Log &) = delete;
    Log(Log &&) = delete;
    Log & operator=(Log &&) = delete;

    static auto& instance(){
        static Log log;
        return log;
    }

    static void console(const QString &rawMessage, bool overrideSilence);
    static void debug(const QString &log, util::source_location location = util::source_location::current());
    static void information(const QString &log, util::source_location location = util::source_location::current());
    static void warning(const QString &log, util::source_location location = util::source_location::current());
    static void error(const QString &log, util::source_location location = util::source_location::current());
    static void critical(const QString &log, util::source_location location = util::source_location::current());

	static void clear();
    static void backupLastLog();

    static QString path();
    static QString pathOld();

    void write(const QString &msg, bool force = false, bool useStdErr = false, LoggingMode mode = LM_UNSPECIFIED);
    void write(const QString &log,
               Severity       severity,
               util::source_location location,
               LoggingMode    mode = LM_UNSPECIFIED);

    bool getColoredOutput() const;
    void setColoredOutput(bool newColoredOutput);

    bool getSilent() const;
    void setSilent(bool newSilent);

    LoggingMode getLoggingMode() const;
    void setLoggingMode(LoggingMode newLoggingMode);

    bool getUseSimpleFormat() const;
    void setUseSimpleFormat(bool newUseSimpleFormat);

    Severity getMinSeverity() const;
    void setMinSeverity(Severity newMinSeverity);

private:
    static QString prepareDebugMessage(const QString &message, util::source_location location);

    bool coloredOutput = true;
    bool silent = false;
    LoggingMode loggingMode = LM_ALL;
    bool useSimpleFormat = false;
    Severity minSeverity = Debug;
};

#endif // LOG_H
