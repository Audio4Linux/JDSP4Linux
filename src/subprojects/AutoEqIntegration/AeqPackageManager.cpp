#include "AeqPackageManager.h"
#include "GzipDownloaderDialog.h"
#include "HttpException.h"

#include "config/AppConfig.h"

#include <http/src/http.h>
#include <QDir>
#include <QFile>

#define REPO_ROOT QString("https://raw.githubusercontent.com/ThePBone/AutoEqPackages/main/")

using namespace QtPromise;

AeqPackageManager::AeqPackageManager(QObject *parent) : QObject(parent)
{

}

QtPromise::QPromise<void> AeqPackageManager::installPackage(AeqVersion version, QWidget* hostWindow)
{
    return QPromise<void>{[&](
        const QtPromise::QPromiseResolve<void>& resolve,
        const QtPromise::QPromiseResolve<void>& reject) {

        auto downloader = new GzipDownloaderDialog(version.toDownloadReply(), targetDirectory(), hostWindow);
        bool success = downloader->exec();
        downloader->deleteLater();

        if(success)
        {
            resolve();
        }
        reject();
    }};
}

bool AeqPackageManager::uninstallPackage()
{
    return QDir(targetDirectory()).removeRecursively();
}

bool AeqPackageManager::isPackageInstalled()
{
    return QDir(targetDirectory()).exists() &&
           QFile(targetDirectory() + "/version.json").exists() &&
           QFile(targetDirectory() + "/index.json").exists();
}

QtPromise::QPromise<AeqVersion> AeqPackageManager::isUpdateAvailable()
{
    return QPromise<AeqVersion>{[&](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve,
        const QtPromise::QPromiseReject<AeqVersion>& reject) {

        QFile versionJson = (targetDirectory() + "/version.json");

        this->getRepositoryVersion().then([&](AeqVersion remote){
            if(!versionJson.exists())
            {
                resolve(remote);
            }

            QJsonDocument d = QJsonDocument::fromJson(versionJson.readAll());
            QJsonArray root = d.array();
            if(root.count() > 0)
            {
                auto local = AeqVersion(root[0].toObject());
                if(remote.commitTime > local.commitTime)
                {
                    // Remote is newer
                    resolve(remote);
                }
            }

            reject();
        }).fail([](const HttpException& error) {
            throw error;
        });
    }};
}

QPromise<AeqVersion> AeqPackageManager::getRepositoryVersion()
{
    return QPromise<AeqVersion>{[&](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve) {

        auto reply = Http::instance().get(QUrl(REPO_ROOT + "/version.json"));
        connect(reply, &HttpReply::finished, this, [resolve](const HttpReply &reply)
        {
            if (reply.isSuccessful())
            {
                QJsonDocument d = QJsonDocument::fromJson(reply.body());
                QJsonArray root = d.array();
                for(const auto& item : root)
                {
                    QJsonObject pkg = item.toObject();
                    QJsonArray types = pkg.value("type").toArray();

                    // Select correct package
                    if(types.contains(QJsonValue("GraphicEQ")) &&
                       types.contains(QJsonValue("CSV")) &&
                       types.count() == 2)
                    {
                        resolve(AeqVersion(pkg));
                    }
                }

                throw new HttpException(900, "Requested package type currently unavailable");
            }
            else
            {
                throw new HttpException(reply);
            }
        });
    }};
}

QString AeqPackageManager::targetDirectory()
{
    return AppConfig::instance().getPath("autoeq");
}
