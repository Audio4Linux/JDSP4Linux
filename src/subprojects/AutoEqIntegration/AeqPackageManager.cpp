#include "AeqPackageManager.h"
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

QtPromise::QPromise<void> AeqPackageManager::installPackage()
{
    return QPromise<void>{[&](
        const QtPromise::QPromiseResolve<void>& resolve) {

        auto reply = Http::instance().get(QUrl(REPO_ROOT + "/version.json"));
        connect(reply, &HttpReply::finished, this, [resolve](HttpReply &reply)
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
                        AeqVersion version;
                        version.commit = pkg.value("commit").toString();
                        version.commitTime = QDateTime::fromString(pkg.value("commit_time").toString(), "yyyy/MM/dd HH:mm:ss");
                        version.commitTime.setTimeSpec(Qt::UTC);
                        version.packageTime = QDateTime::fromString(pkg.value("package_time").toString(), "yyyy/MM/dd HH:mm:ss");
                        version.packageTime.setTimeSpec(Qt::UTC);
                        version.packageUrl = pkg.value("package_url").toString();
                        for(const auto& type : types)
                        {
                            version.type.append(type.toString());
                        }
                        resolve(version);
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

bool AeqPackageManager::isPackageInstalled()
{
    return QDir(targetDirectory()).exists() &&
           QFile(targetDirectory() + "/version.json").exists() &&
           QFile(targetDirectory() + "/index.json").exists();
}

QPromise<AeqVersion> AeqPackageManager::getRepositoryVersion()
{
    return QPromise<AeqVersion>{[&](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve) {

        auto reply = Http::instance().get(QUrl(REPO_ROOT + "/version.json"));
        connect(reply, &HttpReply::finished, this, [resolve](HttpReply &reply)
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
                        AeqVersion version;
                        version.commit = pkg.value("commit").toString();
                        version.commitTime = QDateTime::fromString(pkg.value("commit_time").toString(), "yyyy/MM/dd HH:mm:ss");
                        version.commitTime.setTimeSpec(Qt::UTC);
                        version.packageTime = QDateTime::fromString(pkg.value("package_time").toString(), "yyyy/MM/dd HH:mm:ss");
                        version.packageTime.setTimeSpec(Qt::UTC);
                        version.packageUrl = pkg.value("package_url").toString();
                        for(const auto& type : types)
                        {
                            version.type.append(type.toString());
                        }
                        resolve(version);
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
