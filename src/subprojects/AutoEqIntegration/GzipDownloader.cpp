#include "GzipDownloader.h"

#include "Untar.h"
#include "ExtractionThread.h"

bool GzipDownloader::start(QNetworkReply *reply, QDir _extractionPath)
{
    if(networkReply)
    {
        return false;
    }

    if(!QDir().mkpath(TMP_DIR))
    {
        return false;
    }

    downloadedFile.setFileName(TMP_DIR + QDateTime::currentDateTime().toString("yyyy_MM_dd_hhmmss_zzz") + ".tar.gz");
    if(!downloadedFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
    {
        return false;
    }

    extractionPath = _extractionPath;
    networkReply = reply;
    connect(networkReply, &QIODevice::readyRead, this, &GzipDownloader::onDataAvailable);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(networkReply, &QNetworkReply::errorOccurred, this, &GzipDownloader::onErrorOccurred);
#else
    connect(networkReply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, &GzipDownloader::onErrorOccurred);
#endif
    connect(networkReply, &QNetworkReply::downloadProgress, this, &GzipDownloader::downloadProgressUpdated);
    connect(networkReply, &QNetworkReply::finished, this, &GzipDownloader::onArchiveReady);
    return true;
}

void GzipDownloader::abort()
{
    cleanup();
}

bool GzipDownloader::isActive()
{
    return networkReply;
}

QNetworkAccessManager* GzipDownloader::getManager()
{
    return nam;
}

void GzipDownloader::onDataAvailable()
{
    if(!isActive()) {
        return;
    }

    auto dat = networkReply->readAll();
    downloadedFile.write(dat);
}

void GzipDownloader::onArchiveReady()
{
    if(!isActive()) {
        return;
    }

    if(networkReply->error() != QNetworkReply::NoError)
    {
        emit errorOccurred(networkReply->errorString());
        cleanup();
    }
    else
    {
        auto dat = networkReply->readAll();
        downloadedFile.write(dat);
        downloadedFile.close();

        emit decompressionStarted();

        extractThread = new ExtractionThread(extractionPath.path(), downloadedFile.fileName(), this);
        connect(extractThread, &ExtractionThread::onFinished, this, &GzipDownloader::onArchiveExtracted);
        extractThread->start();
    }
}

void GzipDownloader::onArchiveExtracted(const QString &errorString)
{
    if(errorString.isEmpty())
    {
        emit success();
    }
    else
    {
        emit errorOccurred(errorString);
    }
    cleanup();
}

void GzipDownloader::onErrorOccurred(QNetworkReply::NetworkError ex)
{
    Q_UNUSED(ex)
    // Note: Already handled by finish() signal
    // emit errorOccurred(QVariant::fromValue(ex).toString());
    // cleanup();
}

void GzipDownloader::cleanup()
{
    if(networkReply)
    {
        networkReply->abort();
        networkReply->deleteLater();
        networkReply = nullptr;
    }
    downloadedFile.close();
    downloadedFile.remove();

    if(extractThread) {
        disconnect(extractThread, &ExtractionThread::onFinished, this, &GzipDownloader::onArchiveExtracted);
        extractThread->requestInterruption();
        extractThread->deleteLater();
        extractThread = nullptr;
    }
}
