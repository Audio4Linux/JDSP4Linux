#include "CrashReportDialog.h"
#include "ui_CrashReportDialog.h"

#include <QStyleFactory>
#include <QStyle>

CrashReportDialog::CrashReportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CrashReportDialog)
{
    ui->setupUi(this);
    ui->description->setMinimumHeight(ui->description->fontMetrics().boundingRect(
                                          this->rect(), Qt::TextWordWrap, ui->description->text()).height());
}

CrashReportDialog::~CrashReportDialog()
{
    delete ui;
}
