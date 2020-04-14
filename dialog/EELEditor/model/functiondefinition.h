#ifndef FUNCTIONDEFINITION_H
#define FUNCTIONDEFINITION_H

#include <QObject>
#include <QIcon>

class FunctionDefinition
{
public:
    FunctionDefinition(int _line = -1, QString _name = "",
                       QStringList _parameters = QStringList(),
                       bool _isProcedure = false){
        line = _line;
        name = _name;
        parameters = _parameters;
        isProcedure = _isProcedure;
    }
    ~FunctionDefinition(){}
    QString toString() const{
        QString str;
        for(QString parameter : parameters){
            if(parameters.last() != parameter)
                str += QString("%1 ").arg(parameter);
            else
                str += parameter;
        }
        return QString("%1(%2)").arg(name).arg(str);
    }
    QIcon toIcon(){
        if(isProcedure)
            return QIcon(QPixmap(":/icons/MethodSnippet_16x.svg"));
        return QIcon(QPixmap(":/icons/Method_16x.svg"));
    }
    bool operator==(const FunctionDefinition& def) const
    {
        if(toString() == def.toString())
            return true;
        return false;
    }
    bool hasLocalVariables(){
        return !localVariables.empty();
    }
    bool hasInstanceVariables(){
        return !instanceVariables.empty();
    }
    bool isLineWithinFunction(int linenum){
        if(hasInvalidBrackets())
            return false;
        return (line <= linenum) && (endOfFunction >= linenum);
    }
    bool hasInvalidBrackets(){
        return endOfFunction == -1;
    }
    int line;
    int endOfFunction;
    QString name;
    QStringList parameters;
    QStringList localVariables;
    QStringList instanceVariables;
    bool isProcedure;
};

Q_DECLARE_METATYPE(FunctionDefinition);

#endif // FUNCTIONDEFINITION_H
