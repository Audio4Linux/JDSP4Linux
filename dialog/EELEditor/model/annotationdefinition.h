#ifndef ANNOTATIONDEFINITION_H
#define ANNOTATIONDEFINITION_H

#include <QObject>
#include <QIcon>

class AnnotationDefinition
{
public:
    AnnotationDefinition(int _line = -1, QString _name = ""){
        line = _line;
        name = _name;
    }
    ~AnnotationDefinition(){}
    QString toString() const{
        return name;
    }
    QIcon toIcon(){
        return QIcon(QPixmap(":/icons/Namespace_16x.svg"));
    }
    bool operator==(const AnnotationDefinition& def) const
    {
        if(toString() == def.toString())
            return true;
        return false;
    }
    int line;
    QString name;
};

Q_DECLARE_METATYPE(AnnotationDefinition);

#endif // ANNOTATIONDEFINITION_H
