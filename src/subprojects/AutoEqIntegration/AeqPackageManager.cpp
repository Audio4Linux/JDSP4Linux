#include "AeqPackageManager.h"

#include "config/AppConfig.h"

#include <http/src/http.h>
#include <QDir>
#include <QFile>

#define REPO_ROOT QString("https://raw.githubusercontent.com/ThePBone/AutoEqPackages/main/")

AeqPackageManager::AeqPackageManager(QObject *parent) : QObject(parent)
{

}

bool AeqPackageManager::isPackageInstalled()
{
    return QDir(targetDirectory()).exists() &&
           QFile(targetDirectory() + "/version.json").exists() &&
            QFile(targetDirectory() + "/index.json").exists();
}

QFuture<AeqVersion> AeqPackageManager::getRepositoryVersion()
{
    QPromise<int> promise;
    QFuture<int> future = promise.future();

    auto reply = Http::instance().get(QUrl(REPO_ROOT + "/version.json"));
    connect(reply, &HttpReply::finished, this, [](auto &reply) {
        if (reply.isSuccessful()) {
            qDebug() << "Feel the bytes!" << reply.body();
        } else {
            qDebug() << "Something's wrong here" << reply.statusCode() << reply.reasonPhrase();
        }
    });
}

QString AeqPackageManager::targetDirectory()
{
    return AppConfig::instance().getPath("autoeq");
}
