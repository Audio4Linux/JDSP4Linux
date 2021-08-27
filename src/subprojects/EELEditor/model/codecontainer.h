#ifndef CODECONTAINER_H
#define CODECONTAINER_H

#include <QDebug>
#include <QFile>
#include <QFileDevice>
#include <QString>
#include <QTextStream>

class CodeContainer
{
public:
    CodeContainer(){}
    CodeContainer(QString _path){
        path = _path;
        reloadCode();
    };
    void reloadCode(){
        codeLoaded = true;
        QFile fl(path);
        if (!fl.open(QIODevice::ReadOnly))
            return;
        code = fl.readAll();
    }
    void save(){
        QFile file(path);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            stream << code;
            file.close();;
        }
        else
        {
            qWarning().noquote().nospace() << "CodeContainer::save: " + path + ": " + (file.errorString());
        }
    };

    bool codeLoaded = false;
    QString code;
    QString path;
};

Q_DECLARE_METATYPE(CodeContainer*);

#endif // CODECONTAINER_H
