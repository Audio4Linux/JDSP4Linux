#include "ConfigIO.h"

#include <fstream>
#include <QMessageBox>
#include <QRegularExpression>
#include <string>

QString ConfigIO::writeString(const QVariantMap &map)
{
	QString ret("");

	for (const auto &e : map.keys())
	{
		ret += QString("%1=%2\n").arg(e).arg(map.value(e).toString());
	}

	return ret;
}

void ConfigIO::writeFile(const QString &    path,
                         const QVariantMap &map,
                         const QString &    prefix)
{
	std::ofstream myfile(path.toUtf8().constData());

	if (myfile.is_open())
	{
		if (!prefix.isEmpty())
		{
			myfile << prefix.toUtf8().constData() << std::endl;
		}

		for (const auto &e : map.keys())
		{
			if (map.value(e).type() == (QVariant::Type) QMetaType::Float)
			{
				myfile << e.toStdString() << "=" << QString::number(map.value(e).toDouble(), 'f', 5).toStdString() << std::endl;
			}
			else
			{
				myfile << e.toStdString() << "=" << map.value(e).toString().toStdString() << std::endl;
			}
		}

		myfile.close();
	}
}

QVariantMap ConfigIO::readFile(const QString &path)
{
	QVariantMap   map;
	std::ifstream cFile(path.toUtf8().constData());

	if (cFile.is_open())
	{
		std::string line;

		while (getline(cFile, line))
		{
			QPair<QString, QVariant> out;

			if (readLine(QString::fromStdString(line), out))
			{
				map[out.first] = out.second;
			}
		}

		cFile.close();
	}

	return map;
}

QVariantMap ConfigIO::readString(const QString &string)
{
	QVariantMap map;
	QStringList lines = string.split('\n');

	for (const auto &line : lines)
	{
		QPair<QString, QVariant> out;

		if (readLine(line, out))
		{
			map[out.first] = out.second;
		}
	}

	return map;
}

bool ConfigIO::readLine(const QString &line,
                        QPair<QString, QVariant> &out)
{
    if (line.trimmed().isEmpty() || line.trimmed()[0] == '#' || line.trimmed()[0] == '[')
	{
        return false; // Skip commented lines
	}

	auto    delimiterInlineComment = line.indexOf('#'); // Look for config properties mixed up with comments
	auto    extractedProperty      = line.mid(0, delimiterInlineComment);
	auto    delimiterPos           = extractedProperty.indexOf('=');
	auto    name                   = extractedProperty.mid(0, delimiterPos);
	auto    value                  = extractedProperty.mid(delimiterPos + 1);
	QString qname                  = name.trimmed();
	QString qvalue                 = value.trimmed();

	if (qvalue == "true")
	{
		out = QPair<QString, QVariant>(qname, true);
	}
	else if (qvalue == "false")
	{
		out = QPair<QString, QVariant>(qname, false);
	}
	else
	{
		out = QPair<QString, QVariant>(qname, qvalue);
	}

	return true;
}
