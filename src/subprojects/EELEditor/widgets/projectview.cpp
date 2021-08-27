#include "projectview.h"

#include <QFileSystemModel>

ProjectView::ProjectView(QWidget* parent) :
    QListWidget(parent)
{
    connect(this,&QListWidget::currentItemChanged,this,&ProjectView::updatedCurrentFile);
}

void ProjectView::addFile(QString path){
    closeFile(path);
    CodeContainer* cont = new CodeContainer(path);
    QListWidgetItem* item = new QListWidgetItem();
    QFile file(path);
    item->setText(QFileInfo(file.fileName()).fileName());
    item->setToolTip(path);
    item->setData(Qt::UserRole,QVariant::fromValue<CodeContainer*>(cont));
    item->setSelected(true);
    addItem(item);
    this->setCurrentItem(item);
    selectedItems().clear();
    selectedItems().append(item);
}

void ProjectView::closeFile(QString path){
    for(int i = count() - 1; i >= 0; i--){
        if(path == item(i)->toolTip()){
            takeItem(i);
            break;
        }
    }
}

CodeContainer* ProjectView::getCurrentFile(){
    if(selectedItems().count() < 1)
        return nullptr;

    return selectedItems().first()->data(Qt::UserRole).value<CodeContainer*>();
}

void ProjectView::updatedCurrentFile(QListWidgetItem* item){
    if(item == nullptr) {
        emit currentFileUpdated(previousCont,nullptr);
        return;
    }
    auto container = item->data(Qt::UserRole).value<CodeContainer*>(); 
    emit currentFileUpdated(previousCont,container);
    previousCont = container;
}

void ProjectView::closeCurrentFile(){
    if(selectedItems().count() < 1)
        return;

    QString path = selectedItems().first()->data(Qt::UserRole).value<CodeContainer*>()->path;
    closeFile(path);
}
