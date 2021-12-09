#include "FileDownloaderDialog.h"
#include "ui_FileDownloaderDialog.h"

FileDownloaderDialog::FileDownloaderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileDownloaderDialog)
{
    ui->setupUi(this);
}

FileDownloaderDialog::~FileDownloaderDialog()
{
    delete ui;
}
