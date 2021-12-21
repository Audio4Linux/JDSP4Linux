#include "Log.h"

#include <QDebug>
#include <QFile>
#include <QTime>

#define C_RESET "\u001b[0m"
#define C_RED "\u001b[31m"
#define C_YELLOW "\u001b[33m"
#define C_WHITE "\u001b[37m"
#define C_FAINTWHITE "\u001b[2;37m"
#define C_BACKRED "\u001b[41;37m"

void Log::debug(const QString &log)
{
    write(log, Debug);
}

void Log::information(const QString &log)
{
    write(log, Info);
}

void Log::warning(const QString &log)
{
    write(log, Warning);
}

void Log::error(const QString &log)
{
    write(log, Error);
}

void Log::critical(const QString &log)
{
    write(log, Critical);
}

void Log::write(const QString &log,
                Severity       severity,
                LoggingMode    mode)
{
    QString sev;
    QString color;
    switch(severity)
    {
        case Debug:
            sev = "DBG";
            color = C_FAINTWHITE;
            break;
        case Info:
            sev = "INF";
            color = C_WHITE;
            break;
        case Warning:
            sev = "WRN";
            color = C_YELLOW;
            break;
        case Error:
            sev = "ERR";
            color = C_RED;
            break;
        case Critical:
            sev = "CRT";
            color = C_BACKRED;
            break;
    }

    QFile   file("/tmp/jamesdsp/application.log");
    QString formattedLog(QString("[%1] [%2] %3").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(sev).arg(log));

	if (mode == LM_ALL || mode == LM_FILE)
	{
		if (file.open(QIODevice::WriteOnly | QIODevice::Append))
		{
			file.write(QString("%1\n").arg(formattedLog).toUtf8().constData());
		}

		file.close();
	}

	if (mode == LM_ALL || mode == LM_STDOUT)
	{
        qDebug().noquote().nospace() << color << formattedLog.toUtf8().constData() << C_RESET;
	}
}

void Log::clear()
{
	QFile file("/tmp/jamesdsp/ui.log");

	if (file.exists())
	{
		file.remove();
	}
}
