#include "androidimporterdlg.h"
#include "ui_importandroid.h"
#include "mainwindow.h"
#include "misc/loghelper.h"
#include "config/configconverter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include <sstream>
#include <fstream>
#include <utility> 

using namespace std;
AndroidImporterDlg::AndroidImporterDlg(QString confpath,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::importandroid)
{
    m_confpath = std::move(confpath);
    ui->setupUi(this);
    connect(ui->pushButton,SIGNAL(clicked()),SLOT(import()));
}

AndroidImporterDlg::~AndroidImporterDlg()
{
    delete ui;
}

void AndroidImporterDlg::import(){
    if(ui->lineEdit->text()==""){
        QMessageBox::warning(this, tr("Missing Input"), tr("Name is not set"),QMessageBox::Ok);
        return;
    }
    QString filename = QFileDialog::getOpenFileName(this,tr("Import JamesDSP Android config file (xml)"),"","*.xml");
    if(filename=="")return;

    auto response = ConfigConverter::fromAndroid(filename);
    if( response.failed ) {
        QMessageBox::warning(this, tr("Syntax Error"), response.description,QMessageBox::Ok);
        LogHelper::writeLog("Converter (from android): " + response.description + " (importandroid/syntaxcheck)");
        return;
    }

    string newconfig = response.configuration.toUtf8().constData();
    QString msginfotext = tr("Successfully converted!\n");
    if(response.description.length() > 0){
        msginfotext += tr("\nNotes:\n");
        msginfotext += response.description;
    }

    QDir d = QFileInfo(m_confpath).absoluteDir();
    QString absolute=d.absolutePath();
    QString path = pathAppend(absolute,"presets");

    QString text = ui->lineEdit->text();
    if (!text.isEmpty()) {

        ofstream cfile(QDir::cleanPath(path + QDir::separator() + text + ".conf").toUtf8().constData());
        if (cfile.is_open())
        {
            cfile << newconfig;
            cfile.close();
        }
        else
            LogHelper::writeLog("Unable to create new file at '" + QDir::cleanPath(path + QDir::separator() + text + ".conf") + "'; cannot import converted android config (importandroid/importer)");
    }

    emit importFinished();
    QMessageBox::information(this,tr("Import"),msginfotext);
    this->close();
}
QString AndroidImporterDlg::pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}
