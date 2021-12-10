#include "GzipDownloader.h"

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
    if(!downloadedFile.open(QIODevice::ReadWrite))
    {
        return false;
    }

    extractionPath = _extractionPath;
    networkReply = reply;
    connect(networkReply, &QIODevice::readyRead, this, &GzipDownloader::onDataAvailable);
    connect(networkReply, &QNetworkReply::downloadProgress, this, &GzipDownloader::downloadProgressUpdated);
    connect(networkReply, &QNetworkReply::finished, this, &GzipDownloader::onArchiveReady);
    return true;
}

void GzipDownloader::abort()
{
    if(!networkReply)
    {
        return;
    }

    cleanup();
}

bool GzipDownloader::isActive()
{
    return networkReply;
}

void GzipDownloader::onDataAvailable()
{
    downloadedFile.write(networkReply->readAll());
}

void GzipDownloader::onArchiveReady()
{
    if(networkReply->error() != QNetworkReply::NoError)
    {
        cleanup();
        emit error(networkReply->errorString());
    }
    else
    {
        downloadedFile.write(networkReply->readAll());

        emit decompressionStarted();

        auto promise = QtPromise::resolve(QtConcurrent::run([this]()
        {
            QDir(extractionPath).mkpath(extractionPath.path());

            auto file = gzopen(downloadedFile.fileName().toStdString().c_str(), "rb");
            auto temp = std::tmpfile();

            inflate(file, temp);
            gzclose(file);
            std::rewind(temp);
            untar(temp, extractionPath.path().toStdString().c_str());
            std::fclose(temp);

        })).then([this]
        {
            emit success();
            cleanup();
        });
    }
}

void GzipDownloader::cleanup()
{
    if(networkReply)
    {
        networkReply->abort();
        networkReply->deleteLater();
    }
    downloadedFile.close();
    downloadedFile.remove();
}
