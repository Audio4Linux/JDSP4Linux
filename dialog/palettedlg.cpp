#include "palettedlg.h"
#include "ui_palettedlg.h"

#include <QColorDialog>

PaletteEditor::PaletteEditor(AppConfigWrapper *_appconf, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::palette)
{
    ui->setupUi(this);

    appconf = _appconf;

    ui->whiteicons->setChecked(appconf->getWhiteIcons());

    QObject::connect(ui->base, SIGNAL(clicked()),
                     this, SLOT(updateBase()));
    QObject::connect(ui->back, SIGNAL(clicked()),
                     this, SLOT(updateBack()));
    QObject::connect(ui->fore, SIGNAL(clicked()),
                     this, SLOT(updateFore()));
    QObject::connect(ui->sel, SIGNAL(clicked()),
                     this, SLOT(updateSelection()));
    QObject::connect(ui->disabled, SIGNAL(clicked()),
                     this, SLOT(updateDisabled()));

    connect(ui->close,SIGNAL(clicked()),this,SLOT(closeWin()));
    connect(ui->whiteicons,SIGNAL(clicked()),this,SLOT(updateIcons()));
    connect(ui->reset,SIGNAL(clicked()),this,SLOT(Reset()));
}

PaletteEditor::~PaletteEditor()
{
    delete ui;
}

void PaletteEditor::closeWin(){
    this->close();
}
void PaletteEditor::Reset(){
    appconf->setCustompalette("25,25,25;53,53,53;255,255,255;42,130,218;85,85,85");
}
int PaletteEditor::loadColor(int index,int rgb_index){
    QStringList elements = appconf->getCustompalette().split(';');
    if(elements.length()<5||elements[index].split(',').size()<3){
        if(index==0)return 25;
        else if(index==1)return 53;
        else if(index==2)return 255;
        else if(index==3){
            if(rgb_index==0)return 42;
            else if(rgb_index==1)return 130;
            else if(rgb_index==2)return 218;
        }
        else if(index==4) return 85;
    }
    QStringList rgb = elements[index].split(',');
    return rgb[rgb_index].toInt();
}
void PaletteEditor::saveColor(int index,const QColor& color){
    QString strcolor = QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue());
    QStringList elements = appconf->getCustompalette().split(';');
    while(elements.size()<5)elements.append("");
    elements.replace(index,strcolor);
    appconf->setCustompalette(elements.join(";"));
}
void PaletteEditor::updateIcons(){
    appconf->setWhiteIcons(ui->whiteicons->isChecked());
}
void PaletteEditor::updateBase(){
    QColorDialog *base = new QColorDialog();
    base->setOptions(QColorDialog::DontUseNativeDialog);
    base->setCurrentColor(QColor(loadColor(0,0),loadColor(0,1),loadColor(0,2)));

    if (base->exec() == QColorDialog::Accepted)
    {
        QColor color = base->currentColor();
        saveColor(0,color);
    }
}

void PaletteEditor::updateBack(){
    QColorDialog *back = new QColorDialog();
    back->setOptions(QColorDialog::DontUseNativeDialog);
    back->setCurrentColor(QColor(loadColor(1,0),loadColor(1,1),loadColor(1,2)));

    if (back->exec() == QColorDialog::Accepted)
    {
        QColor color = back->currentColor();
        saveColor(1,color);
    }
}

void PaletteEditor::updateFore(){
    QColorDialog *fore = new QColorDialog();
    fore->setOptions(QColorDialog::DontUseNativeDialog);
    fore->setCurrentColor(QColor(loadColor(2,0),loadColor(2,1),loadColor(2,2)));

    if (fore->exec() == QColorDialog::Accepted)
    {
        QColor color = fore->currentColor();
        saveColor(2,color);
    }
}

void PaletteEditor::updateSelection(){
    QColorDialog *sel = new QColorDialog();
    sel->setOptions(QColorDialog::DontUseNativeDialog);
    sel->setCurrentColor(QColor(loadColor(3,0),loadColor(3,1),loadColor(3,2)));

    if (sel->exec() == QColorDialog::Accepted)
    {
        QColor color = sel->currentColor();
        saveColor(3,color);
    }
}

void PaletteEditor::updateDisabled(){
    QColorDialog *sel = new QColorDialog();
    sel->setOptions(QColorDialog::DontUseNativeDialog);
    sel->setCurrentColor(QColor(loadColor(4,0),loadColor(4,1),loadColor(4,2)));

    if (sel->exec() == QColorDialog::Accepted)
    {
        QColor color = sel->currentColor();
        saveColor(4,color);
    }
}
