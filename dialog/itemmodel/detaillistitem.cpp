#include "detaillistitem.h"
#include "ui_configitem.h"

DetailListItem::DetailListItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::configitem)
{
    ui->setupUi(this);
}

DetailListItem::~DetailListItem()
{
    delete ui;
}

void DetailListItem::setData(QString title, QString desc){
    ui->title->setText(title);
    ui->desc->setText(desc);
}
