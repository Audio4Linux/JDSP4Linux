#ifndef EELPARSER_H
#define EELPARSER_H

#include <model/codecontainer.h>
#include <QMap>

#define NORESULT "<<NO RESULT FOUND>>"

class EELBaseProperty;
enum class EELPropertyType;
typedef QVector<EELBaseProperty*> EELProperties;

class EELParser :
	public QObject
{
	Q_OBJECT

public:
	EELParser();
	void          loadFile(QString path);
	bool          saveFile();
	bool          loadBackup();
	bool          saveBackup();
	bool          deleteBackup();
	bool          backupExists();
	bool          canLoadBackup();
	bool          isFileLoaded();
	QString       getPath();
	QString       getDescription();
	EELProperties getProperties();
	bool          manipulateProperty(EELBaseProperty *prop);

private:
	CodeContainer container;

	QString       findVariable(QString         key,
	                           EELPropertyType type);
	bool          replaceVariable(QString         key,
	                              QString         value,
	                              EELPropertyType type);

};

enum class EELPropertyType
{
	Unknown,
	NumberRange
};

class EELBaseProperty
{
public:
	EELBaseProperty(QString _key,
	                QString _description)
	{
		key         = _key;
		description = _description;
		type        = EELPropertyType::Unknown;
	}

	EELBaseProperty()
	{
		type = EELPropertyType::Unknown;
	};
	virtual ~EELBaseProperty() {};

	QString getKey() const
	{
		return key;
	}

	void setKey(const QString &value)
	{
		key = value;
	}

	QString getDescription() const
	{
		return description;
	}

	void setDescription(const QString &value)
	{
		description = value;
	}

	EELPropertyType getType() const
	{
		return type;
	}

protected:
	QString key;
	QString description;
	EELPropertyType type;
};

template<typename TNum>
class EELNumberRangeProperty :
	public EELBaseProperty
{
public:
	EELNumberRangeProperty(QString _key,
	                       QString _description,
	                       TNum    _value,
	                       TNum    _minimum,
	                       TNum    _maximum,
	                       TNum    _defaultVal = 0)
	{
		key         = _key;
		description = _description;
		value       = _value;
		maximum     = _maximum;
		minimum     = _minimum;
		step        = _defaultVal;
		type        = EELPropertyType::NumberRange;
	}

	EELNumberRangeProperty()
	{
		type = EELPropertyType::NumberRange;
	};
	~EELNumberRangeProperty() {}

	TNum getValue() const
	{
		return value;
	}

	void setValue(const TNum &_value)
	{
		value = _value;
	}

	TNum getMaximum() const
	{
		return maximum;
	}

	void setMaximum(const TNum &value)
	{
		maximum = value;
	}

	TNum getMinimum() const
	{
		return minimum;
	}

	void setMinimum(const TNum &value)
	{
		minimum = value;
	}

	TNum getStep() const
	{
		return step;
	}

	void setStep(const TNum &value)
	{
		step = value;
	}

private:
	TNum maximum;
	TNum minimum;
	TNum step;
	TNum value;
};

#endif // EELPARSER_H
