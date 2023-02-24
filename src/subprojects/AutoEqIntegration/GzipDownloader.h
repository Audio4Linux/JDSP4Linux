#ifndef GZIPDOWNLOADER_H
#define GZIPDOWNLOADER_H

#include <QtWidgets>
#include <QtNetwork>

#include <QtPromise>

#define TMP_DIR "/tmp/jamesdsp/download/"

class ExtractionThread;

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
    QNetworkAccessManager* getManager();

signals:
    void downloadProgressUpdated(qint64 bytesReceived, qint64 bytesTotal);
    void decompressionStarted();
    void success();
    void errorOccurred(QString errorString);

private slots:
    void onDataAvailable();
    void onArchiveReady();
    void onArchiveExtracted(const QString& errorString);
    void onErrorOccurred(QNetworkReply::NetworkError error);

    void cleanup();

private:
    QDir extractionPath;
    QFile downloadedFile;
    QNetworkAccessManager* nam;
    QPointer<QNetworkReply> networkReply = nullptr;
    ExtractionThread *extractThread = nullptr;
};


#endif // GZIPDOWNLOADER_H
