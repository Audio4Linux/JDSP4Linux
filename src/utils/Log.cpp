#include "Log.h"

#include <QDebug>
#include <QFile>
#include <QTime>
#include <filesystem>
#include <stdio.h>

#define C_RESET "\u001b[0m"
#define C_RED "\u001b[31m"
#define C_YELLOW "\u001b[33m"
#define C_CYAN "\u001b[36m"
#define C_WHITE "\u001b[37m"
#define C_FAINTWHITE "\u001b[2;37m"
#define C_BACKRED "\u001b[41;37m"

static QTextStream out(stdout);
static QTextStream err(stderr);

void Log::kernel(const QString &log)
{
    Log::instance().write(log.trimmed(), Kernel, util::source_location::current());
}

void Log::console(const QString &log, bool overrideSilence)
{
    Log::instance().write(log, overrideSilence);
}

void Log::debug(const QString &log, util::source_location location)
{
    Log::instance().write(log, Debug, location);
}

void Log::information(const QString &log, util::source_location location)
{
    Log::instance().write(log, Info, location);
}

void Log::warning(const QString &log, util::source_location location)
{
    Log::instance().write(log, Warning, location);
}

void Log::error(const QString &log, util::source_location location)
{
    Log::instance().write(log, Error, location);
}

void Log::critical(const QString &log, util::source_location location)
{
    Log::instance().write(log, Critical, location);
}

void Log::write(const QString &log,
                Severity       severity,
                util::source_location location,
                LoggingMode    mode)
{
    if(severity < minSeverity)
        return;

    QString msg = useSimpleFormat ? log : prepareDebugMessage(log, location);
    QString sev;
    QString sevSimple;
    QString color;
    bool useStdErr = false;

    switch(severity)
    {
    case Debug:
        sev = "DBG";
        sevSimple = "debug: ";
        color = C_FAINTWHITE;
        break;
    case Info:
        sev = "INF";
        sevSimple = "info: ";
        color = C_WHITE;
        break;
    case Kernel:
        sev = "KNL";
        sevSimple = "kernel: ";
        color = C_CYAN;
        break;
    case Warning:
        sev = "WRN";
        sevSimple = "warning: ";
        color = C_YELLOW;
        useStdErr = true;
        break;
    case Error:
        sev = "ERR";
        sevSimple = "error: ";
        color = C_RED;
        useStdErr = true;
        break;
    case Critical:
        sev = "WTF";
        sevSimple = "critical: ";
        color = C_BACKRED;
        useStdErr = true;
        break;
    }

    QString formattedLog;
    if(useSimpleFormat)
        formattedLog = QString("%1%2").arg(sevSimple).arg(msg);
    else
        formattedLog = QString("[%1] [%2] %3").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(sev).arg(msg);

    if(coloredOutput)
        formattedLog = QString("%1%2%3").arg(color).arg(formattedLog).arg(C_RESET);

    write(formattedLog, false, useStdErr, mode);
}

void Log::write(const QString& msg, bool force, bool useStdErr, LoggingMode mode)
{
    if(mode == LM_UNSPECIFIED)
        mode = loggingMode;

    QFile file(path());
    if (mode == LM_ALL || mode == LM_FILE)
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            file.write(QString("%1\n").arg(msg).toUtf8().constData());
        }

        file.close();
    }

    if ((mode == LM_ALL || mode == LM_STDOUT) && (!silent || force))
    {
        (useStdErr ? err : out) << msg.toUtf8().constData() << Qt::endl;
        (useStdErr ? err : out).flush();
    }
}

QString Log::prepareDebugMessage(const QString& message, util::source_location location)
{
    auto filepath = std::filesystem::path{location.file_name()};
    auto filename = QString::fromStdString(filepath.filename().string());

    auto function = QString::fromStdString(location.function_name());

    // Strip '[with auto:53 = AppConfig::Key; auto:54 = QVariant]' from function signatures with auto params
    if(function.contains("[with")) {
        function = function.mid(0, function.indexOf("[with"));
    }
    // Safely clean up lambda parameter types which may contain '::'
    if(function.contains("<lambda(")) {
        function = function.mid(0, function.indexOf("<lambda(") + 7);
    }

    auto namespaces = function.split("::", Qt::SkipEmptyParts);
    auto actual_function_name = function;
    if(!namespaces.isEmpty()) {
        auto last = namespaces.takeLast();
        if(last.startsWith("<lambda")) {
            /* Nested lambda function, example:
             * PipewireAudioService::PipewireAudioService()::<lambda(const std::string&)>:25) */
            actual_function_name = "";
            if(!namespaces.isEmpty()) {
                // Append parent function if available, but remove arguments
                auto parent_function = namespaces.takeLast();
                auto parent_function_name = parent_function.mid(0, parent_function.indexOf("("));

                // If name is equal to parent namespace of parent function, we've found a constructor
                if(!namespaces.isEmpty() && parent_function_name == namespaces.takeLast()) {
                    parent_function_name.append("$ctor");
                }
                actual_function_name.append(parent_function_name + "::");
            }
            actual_function_name.append("<lambda>");
        }
        else {
            /* Regular function, examples:
             * int {anonymous}::on_metadata_property(void*, uint32_t, const char*, const char*, const char*)
             * void FilterContainer::disconnect_filters() */
            actual_function_name = last.mid(0, last.indexOf("("));

            // If name is equal to parent namespace, we've found a constructor
            if(!namespaces.isEmpty() && actual_function_name == namespaces.takeLast()) {
                actual_function_name.append("$ctor");
            }
        }
    }

    // Clean up return parameters of functions in top-level namespace (like main())
    if(actual_function_name.contains(" ")) {
        actual_function_name = actual_function_name.split(" ").last();
    }

    return filename.split(".").first() + "::" + actual_function_name + ": " + message;
}

Log::Severity Log::getMinSeverity() const
{
    return minSeverity;
}

void Log::setMinSeverity(Severity newMinSeverity)
{
    minSeverity = newMinSeverity;
}

bool Log::getUseSimpleFormat() const
{
    return useSimpleFormat;
}

void Log::setUseSimpleFormat(bool newUseSimpleFormat)
{
    useSimpleFormat = newUseSimpleFormat;
}

Log::LoggingMode Log::getLoggingMode() const
{
    return loggingMode;
}

void Log::setLoggingMode(LoggingMode newLoggingMode)
{
    loggingMode = newLoggingMode;
}

bool Log::getSilent() const
{
    return silent;
}

void Log::setSilent(bool newSilent)
{
    silent = newSilent;
}

bool Log::getColoredOutput() const
{
    return coloredOutput;
}

void Log::setColoredOutput(bool newColoredOutput)
{
    coloredOutput = newColoredOutput;
}

void Log::clear()
{
    QFile file(path());

    if (file.exists())
    {
        file.remove();
    }
}

void Log::backupLastLog()
{
    QFile file(path());
    QFile oldFile(pathOld());

    if (file.exists())
    {
        if(oldFile.exists())
        {
            oldFile.remove();
        }

        file.copy(pathOld());
    }
}

QString Log::path()
{
    return "/tmp/jamesdsp/application.log";
}

QString Log::pathOld()
{
    return "/tmp/jamesdsp/application-prev.log";
}
