#include "logdlg.h"
#include "ui_log.h"

#include <QTextStream>
#include <QFile>

LogDlg::LogDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::log)
{
    ui->setupUi(this);
    connect(ui->reload, SIGNAL(clicked()), this, SLOT(updateLog()));
    connect(ui->select, SIGNAL(currentIndexChanged(int)),this,SLOT(updateLog()));
    updateLog();
}

LogDlg::~LogDlg()
{
    delete ui;
}

void LogDlg::showEvent(QShowEvent *)
{
    updateLog();
}

void LogDlg::updateLog(){
    ui->containerlog->clear();
    QString path;
    if(ui->select->currentText()=="GST Plugin") path = "/tmp/jamesdsp/jdsp.log";
    else path = "/tmp/jamesdsp/ui.log";

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        ui->containerlog->append(tr("<Failed to open '%1'>").arg(path));
    }

    QTextStream in(&file);
    while(!in.atEnd()) {
        QString line = in.readLine();
        ui->containerlog->append(line);
    }

    file.close();
}
