#ifndef GZIPDOWNLOADER_H
#define GZIPDOWNLOADER_H

#include <QtWidgets>
#include <QtNetwork>
#include <QtConcurrent/QtConcurrent>

#include <QtPromise>

#define TMP_DIR "/tmp/jamesdsp/download/"

class GzipDownloader : public QObject
{
    Q_OBJECT
public:
    explicit GzipDownloader(QObject* parent = nullptr) : QObject(parent), nam(new QNetworkAccessManager(parent)){}

    ~GzipDownloader()
    {
        if(networkReply)
        {
            cleanup();
        }
    }

    bool start(QNetworkReply* reply, QDir _extractionPath);
    void abort();
    bool isActive();

signals:
    void downloadProgressUpdated(qint64 bytesReceived, qint64 bytesTotal);
    void decompressionStarted();
    void success();
    void errorOccurred(QString errorString);

private slots:
    void onDataAvailable();
    void onArchiveReady();
    void onErrorOccurred(QNetworkReply::NetworkError error);

    void cleanup();

private:
    QDir extractionPath;
    QFile downloadedFile;
    QNetworkAccessManager* nam;
    QPointer<QNetworkReply> networkReply;
};

#endif // GZIPDOWNLOADER_H
