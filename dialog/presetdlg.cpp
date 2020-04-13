#include "presetdlg.h"
#include "ui_preset.h"
#include "mainwindow.h"
#include "androidimporterdlg.h"
#include "misc/loghelper.h"
#include "misc/common.h"

#include <Animation/Animation.h>

#include <QDir>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>

PresetDlg::PresetDlg(MainWindow* mainwin,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preset)
{
    ui->setupUi(this);
    m_mainwin = mainwin;
    appconf = mainwin->getACWrapper();
    UpdateList();
    connect(ui->add,SIGNAL(clicked()),SLOT(add()));
    connect(ui->load,SIGNAL(clicked()),SLOT(load()));
    connect(ui->remove,SIGNAL(clicked()),SLOT(remove()));
    connect(ui->presetName,SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
    connect(ui->files, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(ui->files, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(presetIndexChanged()));

    QMenu *menu = new QMenu();
    menu->addAction(tr("Android Profile"), this,SLOT(importAndroid()));
    menu->addAction(tr("Linux Configuration"), this,SLOT(importLinux()));
    ui->importBtn->setMenu(menu);

    QMenu *menuEx = new QMenu();
    menuEx->addAction(tr("Linux Configuration"), this,SLOT(exportLinux()));
    ui->exportBtn->setMenu(menuEx);

}
PresetDlg::~PresetDlg()
{
    delete ui;
}

void PresetDlg::reject()
{
    QDialog::reject();
}

void PresetDlg::presetIndexChanged(){
    if(ui->files->currentItem() == nullptr)return;
    ui->presetName->setText(ui->files->currentItem()->text());
}
void PresetDlg::UpdateList(){
    ui->files->clear();
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
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
        placeholder->setText(tr("No presets saved"));
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

void PresetDlg::add(){
    if(ui->presetName->text()==""){
        QMessageBox::warning(this, tr("Error"), tr("Preset Name is empty"),QMessageBox::Ok);
        return;
    }
    m_mainwin->ApplyConfig(false);
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");
    m_mainwin->SavePresetFile(path + "/" + ui->presetName->text() + ".conf");
    ui->presetName->text() = "";
    UpdateList();
    emit presetChanged();
}
void PresetDlg::importAndroid(){
    AndroidImporterDlg* ia = new AndroidImporterDlg(m_mainwin->getACWrapper()->getPath(),this);
    QWidget* host = new QWidget(this);
    host->setProperty("menu", false);
    QVBoxLayout* hostLayout = new QVBoxLayout(host);
    hostLayout->addWidget(ia);
    host->hide();
    WAF::Animation::sideSlideIn(host, WAF::BottomSide);

    connect(ia,&AndroidImporterDlg::importFinished,this,[host,this](){
        WAF::Animation::sideSlideOut(host, WAF::BottomSide);
        UpdateList();
        emit presetChanged();
    });
}
void PresetDlg::importLinux(){
    QString filename = QFileDialog::getOpenFileName(this,tr("Load custom audio.conf"),"","*.conf");
    if(filename=="")return;

    QFileInfo fileInfo(filename);
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");

    const QString& src = filename;
    const QString dest = path + "/" + fileInfo.fileName();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Importing from "+filename+ " (presets/linuximport)");
}
void PresetDlg::exportLinux(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::warning(this, tr("Error"), tr("Nothing selected"),QMessageBox::Ok);
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,tr("Save audio.conf"),"","*.conf");
    if(filename=="")return;
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if(ext!="conf")filename.append(".conf");

    QFileInfo fileInfo(filename);
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    QFile file (fullpath);
    if(!QFile::exists(fullpath)){
        QMessageBox::warning(this, tr("Error"), tr("Selected File doesn't exist"),QMessageBox::Ok);
        UpdateList();
        emit presetChanged();
        return;
    }

    const QString& src = fullpath;
    const QString dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Exporting to "+filename+ " (presets/linuxexport)");
}
void PresetDlg::remove(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::warning(this, tr("Error"), tr("Nothing selected"),QMessageBox::Ok);
        return;
    }
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    QFile file (fullpath);
    if(!QFile::exists(fullpath)){
        QMessageBox::warning(this, tr("Error"), tr("Selected File doesn't exist"),QMessageBox::Ok);
        UpdateList();
        emit presetChanged();
        return;
    }
    file.remove();
    LogHelper::writeLog("Removed "+fullpath+ " (presets/remove)");
    UpdateList();
    emit presetChanged();
}
void PresetDlg::load(){
    if(ui->files->selectedItems().length() == 0){
        QMessageBox::warning(this, tr("Error"), tr("Nothing selected"),QMessageBox::Ok);
        return;
    }
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
    if(!QFile::exists(fullpath)){
        QMessageBox::warning(this, tr("Error"), tr("Selected File doesn't exist"),QMessageBox::Ok);
        UpdateList();
        emit presetChanged();
        return;
    }
    m_mainwin->LoadPresetFile(fullpath);
}
void PresetDlg::nameChanged(const QString& name){
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    if(QFile::exists(path + "/" + name + ".conf"))ui->add->setText(tr("Overwrite"));
    else ui->add->setText(tr("Save"));
}
void PresetDlg::showContextMenu(const QPoint &pos)
{
    QPoint globalPos = ui->files->mapToGlobal(pos);
    QMenu menu;
    QAction* action_rename = menu.addAction(tr("Rename"));
    QAction* action_del = menu.addAction(tr("Delete"));
    QListWidgetItem* pointedItem = ui->files->itemAt(pos);
    if(!pointedItem)return;
    QDir d = QFileInfo(appconf->getPath()).absoluteDir();
    QString path = pathAppend(d.absolutePath(),"presets");
    QString fullpath = QDir(path).filePath(pointedItem->text() + ".conf");


    QAction* selectedAction;
    if(pointedItem){
        selectedAction = menu.exec(globalPos);
        if(selectedAction) {
            if(selectedAction == action_rename) {
                bool ok;
                QString text = QInputDialog::getText(this, tr("Rename"),
                                                     tr("New Name"), QLineEdit::Normal,
                                                     pointedItem->text(), &ok);
                if (ok && !text.isEmpty())QFile::rename(fullpath,QDir(path).filePath(text + ".conf"));
                UpdateList();
                emit presetChanged();
            }
            if(selectedAction == action_del) {
                if(!QFile::exists(fullpath)){
                    QMessageBox::warning(this, tr("Error"), tr("Selected File doesn't exist"),QMessageBox::Ok);
                    UpdateList();
                    emit presetChanged();
                    return;
                }
                QFile file (fullpath);
                file.remove();
                LogHelper::writeLog("Removed "+fullpath);
                UpdateList();
                emit presetChanged();

            }
        }
    }
    menu.exec(globalPos);
}
