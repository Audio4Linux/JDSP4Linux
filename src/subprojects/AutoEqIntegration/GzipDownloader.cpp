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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(networkReply, &QNetworkReply::error, this, &GzipDownloader::onErrorOccurred);
#else
    connect(networkReply, &QNetworkReply::errorOccurred, this, &GzipDownloader::onErrorOccurred);
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

void GzipDownloader::onDataAvailable()
{
    downloadedFile.write(networkReply->readAll());
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

            emit unarchiveStarted();

            bool bad_checksum;
            bool short_read;

            untar(temp, extractionPath.path().toStdString().c_str(), &bad_checksum, &short_read);
            std::fclose(temp);

            if(bad_checksum)
            {
                return "Bad checksum, corrupted package. Please try again.";
            }
            if(short_read)
            {
                return "Short read; expected 512 bytes but received less. Please try again.";
            }
            return "";
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
