#ifndef GZIPDOWNLOADER_H
#define GZIPDOWNLOADER_H

#include <QtWidgets>
#include <QtNetwork>
#include <QtConcurrent/QtConcurrent>

#include <QtPromise>

#define TMP_DIR "/tmp/jamesdsp/download/"
#define CHUNK 16384

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

    /* Decompress from file source to file dest until stream ends or EOF. */
    /*inline void inflate(gzFile_s  *source, FILE *dest)
    {
        unsigned char buf[CHUNK];

        for (
             int size = gzread(source, buf, CHUNK);
             size > 0;
             size = gzread(source, buf, CHUNK)
             ) std::fwrite(buf, 1, CHUNK, dest);
    }*/

private:
    QDir extractionPath;
    QFile downloadedFile;
    QNetworkAccessManager* nam;
    QPointer<QNetworkReply> networkReply;
};

#endif // GZIPDOWNLOADER_H
