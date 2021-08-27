// QCodeEditor
#include <QLanguage.hpp>

// Qt
#include <QStringListModel>
#include <QFile>
#include <QDebug>
#include <QAbstractItemView>
#include <QListView>

#include "eelcompleter.h"

EELCompleter::EELCompleter(QObject *parent) :
    QCompleter(parent)
{
    // Setting up EEL types
    smodel = new QStandardItemModel();
    setModel(smodel);

    Q_INIT_RESOURCE(editorresources);
    QFile fl(":/definitions/eelang.xml");

    if (!fl.open(QIODevice::ReadOnly))
        return;

    QLanguage language(&fl);

    if (!language.isLoaded())
        return;

    int row = 0;
    auto keys = language.keys();
    for (auto&& key : keys)
    {
        auto names = language.names(key);
        for(auto name : names){
            insertBuiltinItem(key,name,row);
            row++;
        }
    }

    smodel->setRowCount(row);
    smodel->setColumnCount(1);

    setModel(smodel);

    this->popup()->setIconSize(QSize(16,16));

    setCompletionColumn(0);
    setCompletionMode(QCompleter::CompletionMode::PopupCompletion);
    setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setCaseSensitivity(Qt::CaseSensitive);
    setWrapAround(true);
}
void EELCompleter::removeCustomItems(){
    for(int i = smodel->rowCount() - 1; i >= 0; i--){
        if(smodel->item(i,0) == nullptr)
            continue;

        QString t = smodel->item(i,0)->toolTip();
        bool builtin = smodel->item(i,0)->data().toBool();

        if(!builtin)
            smodel->removeRow(i);
    }
    smodel->setRowCount(smodel->rowCount()-1);
    setModel(smodel);
}
void EELCompleter::appendDefinitions(QList<FunctionDefinition> defs){
    for(auto def:defs){
        QStandardItem* item = new QStandardItem(def.name);
        item->setIcon(def.toIcon());
        if(def.isProcedure)
            item->setToolTip("User-defined procedure");
        else
            item->setToolTip("User-defined function");
        //Set 'builtin' to false
        item->setData(false);
        item->setData(QVariant::fromValue<FunctionDefinition>(def),Qt::UserRole + 2);

        smodel->setRowCount(smodel->rowCount() + 1);
        smodel->appendRow(item);
    }
    setModel(smodel);
}
void EELCompleter::appendDefinitionsIfMissing(QList<FunctionDefinition> defs){
    QList<FunctionDefinition> toAdd;
    for(auto cdef : defs){
        bool found = false;

        for(int i = smodel->rowCount() - 1; i >= 0; i--){
            if(smodel->item(i,0) == nullptr)
                continue;
            bool builtin = smodel->item(i,0)->data().toBool();
            if(builtin)
                continue;

            QVariant olddef = smodel->item(i,0)->data(Qt::UserRole + 2);
            if(!olddef.canConvert<FunctionDefinition>())
                continue;

            QString oldsignature = olddef.value<FunctionDefinition>().toString();
            if(cdef.toString() == oldsignature){
                found = true;
                break;
            }

        }
        if(!found){
            toAdd.push_back(cdef);
        }
    }
    appendDefinitions(toAdd);
}
void EELCompleter::removeStubbedDefinitions(QList<FunctionDefinition> currentdefs){
    for(int i = smodel->rowCount() - 1; i >= 0; i--){
        if(smodel->item(i,0) == nullptr)
            return;

        bool builtin = smodel->item(i,0)->data().toBool();
        if(builtin)
            continue;
        QVariant olddef = smodel->item(i,0)->data(Qt::UserRole + 2);
        if(!olddef.canConvert<FunctionDefinition>())
            continue;
        QString oldsignature = olddef.value<FunctionDefinition>().toString();
        bool found = false;
        for(auto cdef : currentdefs){
            if(cdef.toString() == oldsignature){
                found = true;
                break;
            }
        }
        if(!found){
            smodel->removeRow(i);
            smodel->setRowCount(smodel->rowCount()-1);
        }
    }
    setModel(smodel);
}
void EELCompleter::insertBuiltinItem(QString key, QString name, int row){
    QStandardItem* item = new QStandardItem(name);
    if(key == "Constant"){
        item->setIcon(QIcon(":icons/Constant_16x.svg"));
        item->setToolTip("In-built constant");
    }
    else if(key == "Function"){
        item->setIcon(QIcon(":icons/Method_16x.svg"));
        item->setToolTip("In-built function");
    }
    else if(key == "Keyword"){
        item->setIcon(QIcon(":icons/Object_16x.svg"));
        item->setToolTip("Keyword");
    }
    else if(key == "PrimitiveType"){
        item->setIcon(QIcon(":icons/Type_16x.svg"));
        item->setToolTip("Basic in-built type");
    }
    else if(key == "Procedure"){
        item->setIcon(QIcon(":icons/MethodSnippet_16x.svg"));
        item->setToolTip("In-built procedure (no-return)");
    }

    //Set 'builtin' to true
    item->setData(true);

    smodel->setRowCount(smodel->rowCount() + 1);
    smodel->setItem(row, 0, item);
    setModel(smodel);
}
