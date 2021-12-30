#include "GzipDownloader.h"

#include "Untar.h"

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

static int bytes = 0;
void GzipDownloader::onDataAvailable()
{
    auto dat = networkReply->readAll();
    downloadedFile.write(dat);
}

void GzipDownloader::onArchiveReady()
{
    if(networkReply->error() != QNetworkReply::NoError)
    {
        cleanup();
        emit errorOccurred(networkReply->errorString());
    }
    else
    {
        auto dat = networkReply->readAll();
        downloadedFile.write(dat);
        downloadedFile.close();

        emit decompressionStarted();

        auto promise = QtPromise::resolve(QtConcurrent::run([this]()
        {
            QDir(extractionPath).mkpath(extractionPath.path());

            QString errorMsg;
            int ret = Untar::extract(downloadedFile.fileName(), extractionPath.path(), errorMsg);
            downloadedFile.remove();

            if(ret > 0)
            {
                return errorMsg;
            }

            return QString("");

        })).then([this](const QString& msg)
        {
            if(msg.isEmpty())
            {
                emit success();
            }
            else
            {
                emit errorOccurred(msg);
            }
            cleanup();
        });
    }
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
    }
    downloadedFile.close();
    downloadedFile.remove();
}
