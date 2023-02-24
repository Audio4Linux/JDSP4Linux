#ifndef GZIPDOWNLOADERDIALOG_H
#define GZIPDOWNLOADERDIALOG_H

#include "AeqStructs.h"
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
    explicit GzipDownloaderDialog(QNetworkReply* _reply, QDir targetDirectory, QWidget *parent = nullptr);
    ~GzipDownloaderDialog();

protected:
    void showEvent(QShowEvent *ev) override;
    void closeEvent(QCloseEvent *ev) override;

private slots:
    void onSuccess();
    void onError(const QString &msg);
    void onDownloadProgressUpdated(qint64 recv, qint64 total);
    void onDecompressionStarted();

private:
    Ui::FileDownloaderDialog *ui;

    GzipDownloader* gzip;

    QNetworkReply* reply;
    QDir targetDirectory;
    bool closeAllowed = true;
};

#endif // GZIPDOWNLOADERDIALOG_H
