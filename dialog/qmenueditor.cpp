#include "qmenueditor.h"
#include "ui_menueditor.h"

#include <QMessageBox>

QMenuEditor::QMenuEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::menueditor)
{
    ui->setupUi(this);
    connect(ui->button_remove,&QToolButton::clicked,[this]{
        int toBeRemoved = ui->list_target->currentRow();
        QList<QAction*>& list = mTarget;
        if ( toBeRemoved >= 0 && toBeRemoved < list.size() )
        {
            list.removeAt(toBeRemoved);
            populateList(mTarget, ui->list_target);
            ui->list_target->setCurrentRow(toBeRemoved-1);
        }
        emit targetChanged();
    });

    connect(ui->button_insert,&QToolButton::clicked,[this]{
        if(ui->list_source->currentItem() == nullptr)
            return;
        bool success = insertAction(ui->list_source->currentItem()->data(Qt::UserRole).value<QAction*>());
        if(!success)
            QMessageBox::warning(this,"Error","Item already inserted");
        emit targetChanged();
    });

    connect(ui->button_up,&QToolButton::clicked,[this]{
        int curr_index = ui->list_target->currentRow();

        QList<QAction*>& list = mTarget;
        if ( curr_index < 1 || curr_index >= list.size() )
            return;

        qSwap(list[curr_index],list[curr_index-1]);

        populateList(mTarget,ui->list_target);
        ui->list_target->setCurrentRow(curr_index-1);
        emit targetChanged();
    });

    connect(ui->button_down,&QToolButton::clicked,[this]{
        int curr_index = ui->list_target->currentRow();

        QList<QAction*>& list = mTarget;
        if ( curr_index < 0 || curr_index >= list.size() - 1 )
            return;

        qSwap(list[curr_index],list[curr_index+1]);

        populateList(mTarget,ui->list_target);
        ui->list_target->setCurrentRow(curr_index+1);
        emit targetChanged();
    });

    connect(ui->button_reset,&QToolButton::clicked,this,&QMenuEditor::resetPressed);

}

QMenuEditor::~QMenuEditor()
{
    delete ui;
}

void QMenuEditor::setSourceMenu(QMenu* source){
    mSource = source->actions();
    populateList(mSource, ui->list_source);
}

void QMenuEditor::setTargetMenu(QMenu* target){
    mTarget = target->actions();
    populateList(mTarget, ui->list_target);
}

void QMenuEditor::populateList(QList<QAction*> menu, QListWidget* list){
    list->clear();
    for(auto action : menu){
        QString desc = "";
        if(action->menu() != nullptr)
            desc = "List";

        if(desc.isEmpty())
            desc = action->text().replace('&',"");
        else
            desc = QString("%1 (%2)").arg(action->text().replace('&',"")).arg(desc);

        QListWidgetItem* item = new QListWidgetItem(action->isSeparator() ? "--Separator--" :
                                                                            desc);
        if(action->isSeparator())
            item->setForeground(palette().color(QPalette::Disabled,QPalette::ColorRole::Text));

        item->setData(Qt::UserRole,QVariant::fromValue<QAction*>(action));
        list->addItem(item);
    }
}

bool QMenuEditor::insertAction(QAction *new_action)
{
    int act_before = ui->list_target->currentRow();
    for(auto item : mTarget){
        if(item->text() == new_action->text() &&
                !item->isSeparator())
            return false;
    }

    if ( new_action )
    {
        QList<QAction*>& list = mTarget;
        if ( act_before >= 0 && act_before < list.size() )
        {
            list.insert(act_before+1,new_action);
            populateList(mTarget,ui->list_target);
            ui->list_target->setCurrentRow(act_before+1);
        }
        else
        {
            list.push_back(new_action);
            populateList(mTarget,ui->list_target);
            ui->list_target->setCurrentRow(list.size()-1);
        }
    }
    return true;
}

QMenu* QMenuEditor::exportMenu(){
    QMenu* menu = new QMenu();
    for(auto item : mTarget){
        if(item->isSeparator())
            menu->addSeparator();
        else
            menu->addAction(item);
    }
    return menu;
}

void QMenuEditor::setIconStyle(bool white){
    QString postfix = white ? "-white" : "";
    QIcon arrowup(QPixmap(QString(":/icons/arrow-up%1.svg").arg(postfix)));
    QIcon arrowdown(QPixmap(QString(":/icons/arrow-down%1.svg").arg(postfix)));
    QIcon arrowleft(QPixmap(QString(":/icons/arrow-left-double%1.svg").arg(postfix)));
    QIcon arrowright(QPixmap(QString(":/icons/arrow-right-double%1.svg").arg(postfix)));
    QIcon reset(QPixmap(QString(":/icons/edit-undo%1.svg").arg(postfix)));
    ui->button_up->setIcon(arrowup);
    ui->button_down->setIcon(arrowdown);
    ui->button_insert->setIcon(arrowright);
    ui->button_remove->setIcon(arrowleft);
    ui->button_reset->setIcon(reset);
}
