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

    clearProperties();

    // Parse parameters

    QRegularExpression descRe(R"((?<var>\w+):(?<def>-?\d+\.?\d*)?<(?<min>-?\d+\.?\d*),(?<max>-?\d+\.?\d*),?(?<step>-?\d+\.?\d*)?>(?<desc>[\s\S][^\n]*))");
    QRegularExpression descListRe(R"((?<var>\w+):(?<def>-?\d+\.?\d*)?<(?<min>-?\d+\.?\d*),(?<max>-?\d+\.?\d*),?(?<step>-?\d+\.?\d*)?\{(?<opt>[^\}]*)\}>(?<desc>[\s\S][^\n]*))");

    for (const auto &line : container.code.split("\n"))
    {
        // List parameters
        {
            auto matchIterator = descListRe.globalMatch(line);

            if (matchIterator.hasNext())
            {
                auto    match = matchIterator.next();
                QString key   = match.captured("var");
                QString min   = match.captured("min");
                QString max   = match.captured("max");
                QString step  = match.captured("step");
                QString opt   = match.captured("opt");
                QString def   = match.captured("def");
                QString desc  = match.captured("desc").trimmed();

                if (step.isEmpty())
                {
                    step = "1";
                }

                QString current = findVariable(key, EELPropertyType::List);

                if (current == NORESULT)
                {
                    break;
                }

                bool defOk = false;
                std::optional<float> defaultValue = def.toFloat(&defOk);
                if(def.isEmpty() || !defOk)
                    defaultValue = std::nullopt;

                EELListProperty *prop = new EELListProperty(key, desc, defaultValue, current.toInt(),
                                                            min.toInt(), max.toInt(), opt.split(',', Qt::SkipEmptyParts));
                properties.append(prop);
                continue;
            }

        }

        // Number range parameters
        {
            auto matchIterator = descRe.globalMatch(line);

            if (matchIterator.hasNext())
            {
                auto    match = matchIterator.next();
                QString key   = match.captured("var");
                QString min   = match.captured("min");
                QString max   = match.captured("max");
                QString step  = match.captured("step");
                QString def   = match.captured("def");
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

                bool defOk = false;
                std::optional<float> defaultValue = def.toFloat(&defOk);
                if(def.isEmpty() || !defOk)
                    defaultValue = std::nullopt;

                EELNumberRangeProperty<float> *prop = new EELNumberRangeProperty<float>(key, desc, defaultValue, current.toFloat(),
                                                                                        min.toFloat(), max.toFloat(),
                                                                                        step.toFloat());
                properties.append(prop);
                continue;
            }
        }
    }

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

bool EELParser::loadDefaults()
{
    if (!isFileLoaded())
    {
        return false;
    }

    for(const auto& prop : qAsConst(properties))
    {
        if(prop->getType() == EELPropertyType::NumberRange)
        {
            auto* nr = dynamic_cast<EELNumberRangeProperty<float>*>(prop);
            nr->setValue(nr->getDefault());
            manipulateProperty(prop);
        }
        else if (prop->getType() == EELPropertyType::List) {
            auto* list = dynamic_cast<EELListProperty*>(prop);
            list->setValue(list->getDefault());
            manipulateProperty(prop);
        }
    }
    return true;
}

bool EELParser::hasDefaultsDefined()
{
    if (!isFileLoaded())
    {
        return false;
    }

    for(const auto& prop : qAsConst(properties))
    {
        if(prop->hasDefault())
        {
            return true;
        }
    }
    return false;
}

bool EELParser::canLoadDefaults()
{
    if (!isFileLoaded())
    {
        return false;
    }

    for(auto* prop : qAsConst(properties))
    {
        if(prop->getType() == EELPropertyType::NumberRange)
        {
            auto* nr = dynamic_cast<EELNumberRangeProperty<float>*>(prop);

            if(prop->hasDefault() && qFloatCompare(nr->getDefault(), nr->getValue()) == false)
            {
                return true;
            }
        }
        else if(prop->getType() == EELPropertyType::List)
        {
            auto* nr = dynamic_cast<EELListProperty*>(prop);

            if(prop->hasDefault() && nr->getDefault() != nr->getValue())
            {
                return true;
            }
        }
    }

    return false;
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

    for (const auto &line : container.code.split("\n"))
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
    return properties;
}

bool EELParser::manipulateProperty(EELBaseProperty *propbase)
{
    if (propbase->getType() == EELPropertyType::NumberRange)
    {
        EELNumberRangeProperty<float> *prop = dynamic_cast<EELNumberRangeProperty<float>*>(propbase);
        QString                        value;

        if (std::floor(prop->getStep()) == prop->getStep()) // is integer?
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
    else if (propbase->getType() == EELPropertyType::List)
    {
        EELListProperty *prop = dynamic_cast<EELListProperty*>(propbase);
        QString          value = QString::number((int) prop->getValue());

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
    if (type == EELPropertyType::NumberRange || type == EELPropertyType::List)
    {
        QRegularExpression re(QString(R"(%1\s*=\s*(?<val>-?\d+\.?\d*)\s*;)").arg(key));

        for (const auto &line : container.code.split("\n"))
        {
            auto matchIterator = re.globalMatch(line);

            if (matchIterator.hasNext())
            {
                auto match = matchIterator.next();
                return match.captured("val");
            }
        }
    }

    Log::warning(QString("Unable to find a supported variable definition of '%1' in script '%2'").arg(key).arg(getDescription()));
    return NORESULT;
}

bool EELParser::replaceVariable(QString         key,
                                QString         value,
                                EELPropertyType type)
{
    if (type == EELPropertyType::NumberRange || type == EELPropertyType::List)
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

void EELParser::clearProperties()
{
    for(auto& prop : properties)
    {
        delete prop;
        prop = nullptr;
    }
    properties.clear();
}

