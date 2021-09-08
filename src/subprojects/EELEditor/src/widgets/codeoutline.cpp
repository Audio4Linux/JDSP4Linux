#include "codeoutline.h"

CodeOutline::CodeOutline(QWidget *parent)
    : QTreeWidget(parent)
{
    this->setHeaderHidden(true);

    twAnnotation = new QTreeWidgetItem(this);
    twAnnotation->setText(0,"Annotations");
    twAnnotation->setExpanded(true);
    addTopLevelItem(twAnnotation);
    twFunctions = new QTreeWidgetItem(this);
    twFunctions->setText(0,"Functions");
    twFunctions->setExpanded(true);
    addTopLevelItem(twFunctions);

    connect(this,&QTreeWidget::itemSelectionChanged,[this](){
        if(selectedItems().count() < 1 || disableChangeSignal)
            return;
        QTreeWidgetItem* item = selectedItems().first();
        if(item != nullptr){
            auto it = item->data(0,Qt::UserRole);
            if(!it.isValid())
                return;
            //Procedures are handled as functions
            if(it.canConvert<FunctionDefinition>())
                emit functionSelected(it.value<FunctionDefinition>());
            else if(it.canConvert<AnnotationDefinition>())
                emit annotationSelected(it.value<AnnotationDefinition>());
        }
    });
}

void CodeOutline::updateFunctions(QList<FunctionDefinition> defs)
{
    updateField(twFunctions,defs);
}

void CodeOutline::updateAnnotations(QList<AnnotationDefinition> defs)
{
    updateField(twAnnotation,defs);
}

template <typename T>
void CodeOutline::updateField(QTreeWidgetItem* rootView, QList<T> defs)
{
    disableChangeSignal = true;
    bool isExpanded = rootView->isExpanded();
    QString previousSelection;
    if(selectedItems().count() >= 1)
        previousSelection = selectedItems().first()->text(0);

    foreach(auto i, rootView->takeChildren()) delete i;
    for(auto def : defs){
        QTreeWidgetItem* child = new QTreeWidgetItem(rootView);
        child->setText(0,def.toString());
        child->setIcon(0,def.toIcon());
        child->setData(0,Qt::UserRole,QVariant::fromValue<T>(def));
        if(def.toString() == previousSelection){
            child->setSelected(true);
            this->setCurrentItem(child);
        }
        rootView->addChild(child);
    }

    rootView->setSelected(false);
    rootView->setExpanded(isExpanded);
    disableChangeSignal = false;
}

bool CodeOutline::goToAnnotation(QString name){
    this->setCurrentItem(twAnnotation);
    QList<QTreeWidgetItem*> clist = findItems(name, Qt::MatchStartsWith|Qt::MatchRecursive, 0);
    foreach(QTreeWidgetItem* item, clist)
    {
        QVariant data = item->data(0,Qt::UserRole);
        if(data.canConvert<AnnotationDefinition>()){
            this->setCurrentItem(item);
            return true;
        }
    }
    return false;
}

bool CodeOutline::goToFunction(QString name){
    QTreeWidgetItem* rootItem = topLevelItem(1);
    this->setCurrentItem(twFunctions);

    for(int i = 0; i < rootItem->childCount(); i++){
        auto child = rootItem->child(i);

        QVariant data = child->data(0,Qt::UserRole);
        if(data.canConvert<FunctionDefinition>()){
            QString def_name = data.value<FunctionDefinition>().name;
            if(def_name == name){
                this->setCurrentItem(child);
                return true;
            }
        }
    }
    return false;
}

bool CodeOutline::goToFunctionByLine(int line, bool noSync){
    if(noSync)
        disableChangeSignal = true;

    QTreeWidgetItem* rootItem = topLevelItem(1);
    for(int i = 0; i < rootItem->childCount(); i++){
        auto child = rootItem->child(i);

        QVariant data = child->data(0,Qt::UserRole);
        if(data.canConvert<FunctionDefinition>()){
            FunctionDefinition def = data.value<FunctionDefinition>();
            if(def.isLineWithinFunction(line)){
                this->setCurrentItem(child);
                if(noSync)
                    disableChangeSignal = false;
                return true;
            }
        }
    }
    selectionModel()->clear();
    if(noSync)
        disableChangeSignal = false;
    return false;
}
