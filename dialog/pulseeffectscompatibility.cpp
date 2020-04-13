#include "pulseeffectscompatibility.h"
#include "ui_pulseeffectscompatibility.h"
#include "misc/common.h"
#include "misc/autostartmanager.h"
#include "mainwindow.h"

#include <QTimer>
#include <QEasingCurve>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

PulseeffectsCompatibility::PulseeffectsCompatibility(AppConfigWrapper* _appconf, MainWindow*, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PulseeffectsCompatibility)
{
    ui->setupUi(this);

    appconf = _appconf;
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget->setAnimation(QEasingCurve::Type::OutCirc);
    connect(ui->p1_next,&QPushButton::clicked,[=]{
        QString absolute =
                QFileInfo(appconf->getPath()).absoluteDir().absolutePath();
        QString devices(pathAppend(absolute,"devices.conf"));
        if(ui->p2_dev_select->currentData() == "---"){
            QMessageBox::warning(this,"Warning","Invalid device! Please select another one.");
            return;
        }

        ConfigContainer* devconf = new ConfigContainer();
        devconf->setConfigMap(ConfigIO::readFile(devices));
        devconf->setValue("location",ui->p2_dev_select->currentData());
        ConfigIO::writeFile(devices,devconf->getConfigMap());

        ui->stackedWidget->slideInIdx(1);
    });

    connect(ui->p2_next,&QPushButton::clicked,[=]{
        ui->stackedWidget->slideInIdx(2);
    });
    connect(ui->p3_next,&QPushButton::clicked,[=]{
        emit wizardFinished();
    });
    connect(ui->p1_cancel,&QPushButton::clicked,[=]{
        emit wizardFinished();
    });

    refreshDevices();
}

PulseeffectsCompatibility::~PulseeffectsCompatibility()
{
    delete ui;
}

void PulseeffectsCompatibility::refreshDevices()
{
    lockslot = true;
    ui->p2_dev_select->clear();
    QString absolute =
            QFileInfo(appconf->getPath()).absoluteDir().absolutePath();
    QFile devices(pathAppend(absolute,"devices.conf"));

    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("sh", QStringList()<<"-c"<<"pactl list sinks | grep \'Name: \' -A1");
    process.waitForFinished(500);

    ConfigContainer* devconf = new ConfigContainer();
    devconf->setConfigMap(ConfigIO::readFile(pathAppend(absolute,"devices.conf")));
    QString out = process.readAllStandardOutput();
    ui->p2_dev_select->addItem("...","---");
    for(auto item : out.split("Name:")){
        item.prepend("Name:");
        QRegularExpression re(R"((?<=(Name:)\s)(?<name>.+)[\s\S]+(?<=(Description:)\s)(?<desc>.+))");
        QRegularExpressionMatch match = re.match(item, 0, QRegularExpression::PartialPreferCompleteMatch);
        if(match.hasMatch()){
            ui->p2_dev_select->addItem(QString("%1 (%2)").arg(match.captured("desc")).arg(match.captured("name")),
                                       match.captured("name"));
        }
    }
    QString dev_location = devconf->getVariant("location",true).toString();
    if(dev_location.isEmpty())
        ui->p2_dev_select->setCurrentIndex(0);
    else{
        bool notFound = true;
        for(int i = 0; i < ui->p2_dev_select->count(); i++){
            if(ui->p2_dev_select->itemData(i) ==
                    dev_location){
                notFound = false;
                ui->p2_dev_select->setCurrentIndex(i);
                break;
            }
        }
        if(notFound){
            QString name = QString("Unknown (%1)").arg(dev_location);
            ui->p2_dev_select->addItem(name,dev_location);
            ui->p2_dev_select->setCurrentText(name);
        }
    }
    lockslot = false;
}
