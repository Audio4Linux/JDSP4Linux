#include "GzipDownloaderDialog.h"
#include "ui_FileDownloaderDialog.h"

GzipDownloaderDialog::GzipDownloaderDialog(QNetworkReply* _reply, QDir _targetDirectory, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileDownloaderDialog),
    gzip(this)
{
    ui->setupUi(this);

    ui->size->setText("");
    ui->progress->setValue(0);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, &gzip, &GzipDownloader::abort);
    connect(&gzip, &GzipDownloader::success, this, &QDialog::accept);

    connect(&gzip, &GzipDownloader::error, this, &GzipDownloaderDialog::onError);
    connect(&gzip, &GzipDownloader::downloadProgressUpdated, this, &GzipDownloaderDialog::onDownloadProgressUpdated);
    connect(&gzip, &GzipDownloader::decompressionStarted, this, &GzipDownloaderDialog::onDecompressionStarted);

    reply = _reply;
    targetDirectory = _targetDirectory;
}

GzipDownloaderDialog::~GzipDownloaderDialog()
{
    delete ui;
}

void GzipDownloaderDialog::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    gzip.start(reply, targetDirectory);
}

void GzipDownloaderDialog::onError(const QString& msg)
{
    QMessageBox::critical(this, "Error", msg);
    this->reject();
}

void GzipDownloaderDialog::onDownloadProgressUpdated(qint64 recv, qint64 total)
{

    ui->progress->setValue((double)recv/total * 100);
    ui->size->setText(QString("%1MB of %2MB downloaded")
                      .arg(QString::number(recv/1000000.f, 'g', 2))
                      .arg(QString::number(total/1000000.f, 'g', 2)));
}

void GzipDownloaderDialog::onDecompressionStarted()
{
    ui->progress->setMinimum(0);
    ui->progress->setMaximum(0);
    ui->title->setText("Decompressing package...");
    ui->size->setText("");
    ui->buttonBox->setEnabled(false);
}
