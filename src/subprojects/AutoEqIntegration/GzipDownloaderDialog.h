#ifndef GZIPDOWNLOADERDIALOG_H
#define GZIPDOWNLOADERDIALOG_H

#include "GzipDownloader.h"

#include <QDialog>

class QNetworkReply;

namespace Ui {
class FileDownloaderDialog;
}

class GzipDownloaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GzipDownloaderDialog(QNetworkReply* reply, QDir targetDirectory, QWidget *parent = nullptr);
    ~GzipDownloaderDialog();

protected:
    void showEvent(QShowEvent *ev) override;

private slots:
    void onError(const QString &msg);
    void onDownloadProgressUpdated(qint64 recv, qint64 total);
    void onDecompressionStarted();
private:
    Ui::FileDownloaderDialog *ui;

    GzipDownloader gzip;

    QNetworkReply* reply;
    QDir targetDirectory;
};

#endif // GZIPDOWNLOADERDIALOG_H
