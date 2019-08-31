#include "preset.h"
#include "ui_preset.h"
#include "main.h"
#include <QDir>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <QString>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <iostream>
#include <sstream>
#include <fstream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegExp>
#include <QJsonObject>
#include <QVector>
#include <QTimer>
#include <QDesktopServices>
Preset::Preset(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preset)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager(this);
    UpdateList();
    connect(ui->add,SIGNAL(clicked()),SLOT(add()));
    connect(ui->load,SIGNAL(clicked()),SLOT(load()));
    connect(ui->remove,SIGNAL(clicked()),SLOT(remove()));
    connect(ui->presetName,SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
    connect(ui->files, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    QMenu *menu = new QMenu();
    menu->addAction("Linux Configuration", this,SLOT(importLinux()));
    ui->importBtn->setMenu(menu);

    QMenu *menuEx = new QMenu();
    menuEx->addAction("Linux Configuration", this,SLOT(exportLinux()));
    ui->exportBtn->setMenu(menuEx);

}
Preset::~Preset()
{
    delete ui;
}

void Preset::reject()
{
    mainwin->enablePresetBtn(true);
    QDialog::reject();
}
void Preset::UpdateList(){
    ui->files->clear();
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");

    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(".");

    QStringList nameFilter("*.conf");
    QStringList files = dir.entryList(nameFilter);
    if(files.count()<1){
        QFont font;
        font.setItalic(true);
        font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("No presets saved");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->files->addItem(placeholder);
        return;
    }

    for(int i = 0; i < files.count(); i++){ //Strip extensions
        QFileInfo fi(files[i]);
        files[i] = fi.completeBaseName();
    }

    ui->files->addItems(files);
}
QString Preset::toCamelCase(const QString& s)
{
    QStringList parts = s.split(' ', QString::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());

    return parts.join(" ");
}
QString Preset::pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}
void Preset::add(){
    if(ui->presetName->text()==""){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Preset Name is empty",QMessageBox::Ok);
        return;
    }
    mainwin->ConfirmConf(false);
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");
    mainwin->SavePresetFile(path + "/" + ui->presetName->text() + ".conf");
    ui->presetName->text() = "";
    UpdateList();
}
void Preset::importLinux(){
    QString filename = QFileDialog::getOpenFileName(this,"Load custom audio.conf","","*.conf");
    if(filename=="")return;

    QFileInfo fileInfo(filename);
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");

    const QString& src = filename;
    const QString dest = path + "/" + fileInfo.fileName();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    mainwin->writeLog("Importing from "+filename+ " (presets/linuximport)");
    UpdateList();
}
void Preset::exportLinux(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Nothing selected",QMessageBox::Ok);
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,"Save audio.conf","","*.conf");
    if(filename=="")return;
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if(ext!="conf")filename.append(".conf");

    QFileInfo fileInfo(filename);
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    QFile file (fullpath);
    if(!QFile::exists(fullpath)){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Selected File doesn't exist",QMessageBox::Ok);
        UpdateList();
        return;
    }

    const QString& src = fullpath;
    const QString dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    mainwin->writeLog("Exporting to "+filename+ " (presets/linuxexport)");
}
void Preset::remove(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Nothing selected",QMessageBox::Ok);
        return;
    }
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    QFile file (fullpath);
    if(!QFile::exists(fullpath)){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Selected File doesn't exist",QMessageBox::Ok);
        UpdateList();
        return;
    }
    file.remove();
    mainwin->writeLog("Removed "+fullpath+ " (presets/remove)");
    UpdateList();
}
void Preset::load(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Nothing selected",QMessageBox::Ok);
        return;
    }
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    if(!QFile::exists(fullpath)){
        QMessageBox::StandardButton msg;
        msg = QMessageBox::warning(this, "Error", "Selected File doesn't exist",QMessageBox::Ok);
        UpdateList();
        return;
    }
    mainwin->LoadPresetFile(fullpath);
}
void Preset::nameChanged(const QString& name){
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    if(QFile::exists(path + "/" + name + ".conf"))ui->add->setText("Overwrite");
    else ui->add->setText("Save");
}
void Preset::showContextMenu(const QPoint &pos)
{
    QPoint globalPos = ui->files->mapToGlobal(pos);
    QMenu menu;
    QAction* action_rename = menu.addAction("Rename");
    QAction* action_del = menu.addAction("Delete");
    QListWidgetItem* pointedItem = ui->files->itemAt(pos);
    if(!pointedItem)return;
    QDir d = QFileInfo(QString::fromStdString(mainwin->getPath())).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(pointedItem->text() + ".conf");


    QAction* selectedAction;
    if(pointedItem){
        selectedAction = menu.exec(globalPos);
        if(selectedAction) {
            if(selectedAction == action_rename) {
                bool ok;
                QString text = QInputDialog::getText(this, "Rename",
                                                     "New Name", QLineEdit::Normal,
                                                     pointedItem->text(), &ok);
                if (ok && !text.isEmpty())QFile::rename(fullpath,QDir(path).filePath(text + ".conf"));
                UpdateList();
            }
            if(selectedAction == action_del) {
                if(!QFile::exists(fullpath)){
                    QMessageBox::StandardButton msg;
                    msg = QMessageBox::warning(this, "Error", "Selected File doesn't exist",QMessageBox::Ok);
                    UpdateList();
                    return;
                }
                QFile file (fullpath);
                file.remove();
                mainwin->writeLog("Removed "+fullpath);
                UpdateList();
            }
        }
    }
    menu.exec(globalPos);
}
