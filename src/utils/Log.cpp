#include "Log.h"

#include <QDebug>
#include <QFile>
#include <QTime>

void Log::debug(const QString &log)
{
    write(QString("[DBG] %1").arg(log));
}

void Log::information(const QString &log)
{
    write(QString("[INF] %1").arg(log));
}

void Log::warning(const QString &log)
{
    write(QString("[WRN] %1").arg(log));
}

void Log::error(const QString &log)
{
    write(QString("[ERR] %1").arg(log));
}

void Log::critical(const QString &log)
{
    write(QString("[CRT] %1").arg(log));
}

void Log::write(const QString &log,
                LoggingMode    mode)
{
	QFile   file("/tmp/jamesdsp/ui.log");
    QString formattedLog(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss.zzz")).arg(log));

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
		qDebug().noquote().nospace() << formattedLog.toUtf8().constData();
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
