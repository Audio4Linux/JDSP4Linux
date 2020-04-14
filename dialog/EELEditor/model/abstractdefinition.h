#ifndef ABSTRACTDEFINITION_H
#define ABSTRACTDEFINITION_H

#include <QObject>

class AbstractDefinition
{
public:
    AbstractDefinition(){}
    virtual ~AbstractDefinition(){};
    virtual QString toString(){
        return "";
    };
};

Q_DECLARE_METATYPE(AbstractDefinition);


#endif // ABSTRACTDEFINITION_H
