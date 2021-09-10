#include "EelParser.h"
#include "utils/Common.h"
#include "utils/Log.h"

#include <QFileInfo>
#include <QRegularExpression>

EELParser::EELParser()
{}

void EELParser::loadFile(QString path)
{
	container.path = path;
	container.code = "";
	container.reloadCode();
}

bool EELParser::saveFile()
{
	if (!isFileLoaded())
	{
		return false;
	}

	container.save();
	return true;
}

bool EELParser::loadBackup()
{
	if (!backupExists())
	{
		return false;
	}

	QString absolutePath = QFileInfo(container.path).absoluteDir().absolutePath();
	QString fileName     = QFileInfo(container.path).fileName();
	QFile   backup(absolutePath + "/." + fileName + ".bak");

	if (!backup.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QFile script(container.path);

	if (script.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&script);
		stream << backup.readAll();
		script.close();
	}

	backup.remove();
	return true;
}

bool EELParser::saveBackup()
{
	if (!isFileLoaded())
	{
		return false;
	}

	QString absolutePath = QFileInfo(container.path).absoluteDir().absolutePath();
	QString fileName     = QFileInfo(container.path).fileName();
	QFile   file(absolutePath + "/." + fileName + ".bak");

	if (file.exists())
	{
		return false; // We already have a backup

	}

	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);
		stream << container.code;
		file.close();
		return true;
	}

	return false;
}

bool EELParser::deleteBackup()
{
	QString absolutePath = QFileInfo(container.path).absoluteDir().absolutePath();
	QString fileName     = QFileInfo(container.path).fileName();
	QFile   backup(absolutePath + "/." + fileName + ".bak");

	if (!backup.exists())
	{
		return false;
	}

	backup.remove();
	return true;
}

bool EELParser::backupExists()
{
	if (!isFileLoaded())
	{
		return false;
	}

	QString absolutePath = QFileInfo(container.path).absoluteDir().absolutePath();
	QString fileName     = QFileInfo(container.path).fileName();
	QFile   file(absolutePath + "/." + fileName + ".bak");

	return file.exists();
}

bool EELParser::canLoadBackup()
{
	if (!isFileLoaded())
	{
		return false;
	}

	QString absolutePath = QFileInfo(container.path).absoluteDir().absolutePath();
	QString fileName     = QFileInfo(container.path).fileName();
	QFile   file(absolutePath + "/." + fileName + ".bak");

	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	return file.exists() && !(file.readAll() == container.code);
}

bool EELParser::isFileLoaded()
{
	return container.codeLoaded;
}

QString EELParser::getPath()
{
	return container.path;
}

QString EELParser::getDescription()
{
	QRegularExpression descRe(R"((?:^|(?<=\n))(?:desc:)([\s\S][^\n]*))");

	for (auto line : container.code.split("\n"))
	{
		auto matchIterator = descRe.globalMatch(line);

		if (matchIterator.hasNext())
		{
			auto match = matchIterator.next();
			return match.captured(1).trimmed();
		}
	}

	return QFileInfo(container.path).fileName();
}

EELProperties EELParser::getProperties()
{
	EELProperties      props;
	QRegularExpression descRe(R"((?<var>\w+):(?<cur>-?\d+\.?\d*)?<(?<min>-?\d+\.?\d*),(?<max>-?\d+\.?\d*),?(?<step>-?\d+\.?\d*)?>(?<desc>[\s\S][^\n]*))");

	for (auto line : container.code.split("\n"))
	{
		auto matchIterator = descRe.globalMatch(line);

		if (matchIterator.hasNext())
		{
			auto    match = matchIterator.next();
			QString key   = match.captured("var");
			QString min   = match.captured("min");
			QString max   = match.captured("max");
			QString step  = match.captured("step");
			QString desc  = match.captured("desc").trimmed();

			if (step.isEmpty())
			{
				step = "0.1";
			}

			QString current = findVariable(key, EELPropertyType::NumberRange);

			if (current == NORESULT)
			{
				break;
			}

			EELNumberRangeProperty<float> *prop = new EELNumberRangeProperty<float>(key, desc, current.toFloat(),
			                                                                        min.toFloat(), max.toFloat(),
			                                                                        step.toFloat());
			props.append(prop);
		}
	}

	return props;
}

bool EELParser::manipulateProperty(EELBaseProperty *propbase)
{
	saveBackup();

	if (propbase->getType() == EELPropertyType::NumberRange)
	{
		EELNumberRangeProperty<float> *prop = dynamic_cast<EELNumberRangeProperty<float>*>(propbase);
		QString                        value;

		if (is_integer(prop->getStep()))
		{
			value = QString::number((int) prop->getValue());
		}
		else
		{
			value = QString::number(prop->getValue(), 'f', 2);
		}

		bool replace_res = replaceVariable(prop->getKey(), value, prop->getType());
		bool save_res    = saveFile();
		return replace_res && save_res;
	}

	return false;
}

// --- Private members

QString EELParser::findVariable(QString         key,
                                EELPropertyType type)
{
	if (type == EELPropertyType::NumberRange)
	{
		QRegularExpression re(QString(R"(%1\s*=\s*(?<val>-?\d+\.?\d*)\s*;)").arg(key));

		for (auto line : container.code.split("\n"))
		{
			auto matchIterator = re.globalMatch(line);

			if (matchIterator.hasNext())
			{
				auto match = matchIterator.next();
				return match.captured("val");
			}
		}
	}

    Log::warning(QString("EELParser::findVariable: Unable to find a supported variable definition of '%1' in script '%2'").arg(key).arg(getDescription()));
	return NORESULT;
}

bool EELParser::replaceVariable(QString         key,
                                QString         value,
                                EELPropertyType type)
{
	if (type == EELPropertyType::NumberRange)
	{
		QRegularExpression re(QString(R"(%1\s*=\s*(?<val>-?\d+\.?\d*)\s*;)").arg(key));
		auto               matchIterator = re.globalMatch(container.code);

		if (matchIterator.hasNext())
		{
			auto match = matchIterator.next();
			int  start = match.capturedStart("val");
			int  len   = match.capturedLength("val");
			container.code.remove(start, len);
			container.code.insert(start, value);
			return true;
		}
	}

	return false;
}
