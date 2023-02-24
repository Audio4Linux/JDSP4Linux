#include "AeqPackageManager.h"
#include "GzipDownloaderDialog.h"
#include "HttpException.h"

#include "config/AppConfig.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>

#define REPO_ROOT QString("https://raw.githubusercontent.com/ThePBone/AutoEqPackages/main/")

using namespace QtPromise;

AeqPackageManager::AeqPackageManager(QObject *parent) : QObject(parent), nam(new QNetworkAccessManager(this))
{
    nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

QtPromise::QPromise<void> AeqPackageManager::installPackage(AeqVersion version, QWidget* hostWindow)
{
    return QPromise<void>{[this, version, hostWindow](
        const QtPromise::QPromiseResolve<void>& resolve,
                const QtPromise::QPromiseReject<void>& reject) {

            auto reply = nam->get(QNetworkRequest(QUrl(version.packageUrl)));
            auto downloader = new GzipDownloaderDialog(reply, databaseDirectory(), hostWindow);
            bool success = downloader->exec();
            downloader->deleteLater();

            if(success)
                resolve();
            else
                reject();
        }};
}

bool AeqPackageManager::uninstallPackage()
{
    return QDir(databaseDirectory()).removeRecursively();
}

bool AeqPackageManager::isPackageInstalled()
{
    return QDir(databaseDirectory()).exists() &&
            QFile(databaseDirectory() + "/version.json").exists() &&
            QFile(databaseDirectory() + "/index.json").exists();
}

QtPromise::QPromise<AeqVersion> AeqPackageManager::isUpdateAvailable()
{
    return QPromise<AeqVersion>{[this](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve,
        const QtPromise::QPromiseReject<AeqVersion>& reject) {

            QtPromisePrivate::qtpromise_defer([=, this]() {

                this->getRepositoryVersion().then([=, this](AeqVersion remote){
                    this->getLocalVersion().then([=](AeqVersion local){
                        if(remote.packageTime > local.packageTime)
                        {
                            // Remote is newer
                            resolve(remote);
                        }
                        else
                        {
                            reject(remote);
                        }
                    }).fail([=]{
                        // Local file not available, choose remote
                        resolve(remote);
                    });
                }).fail([=](const HttpException& error) {
                    // API error
                    reject(error);
                });
            });
        }};
}

QPromise<AeqVersion> AeqPackageManager::getRepositoryVersion()
{
    return QPromise<AeqVersion>{[&](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve,
                const QtPromise::QPromiseReject<AeqVersion>& reject) {

            auto reqProto = QNetworkRequest(QUrl(REPO_ROOT + "/version.json"));
            reqProto.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);

            QtPromise::connect(nam, &QNetworkAccessManager::finished).then([=](QNetworkReply *reply)
            {
                if(reply->error() != QNetworkReply::NoError)
                {
                    throw HttpException(1, reply->errorString());
                }

                QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

                if (statusCode.toInt() == 200)
                {
                    QJsonDocument d = QJsonDocument::fromJson(reply->readAll());
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

                    throw HttpException(900, "Requested package type currently unavailable");
                }
                else
                {
                    throw HttpException(statusCode.toInt(), reason.toString());
                }
            }).fail([reject](const HttpException& error) {
                reject(error);
            });

            nam->get(reqProto);
        }
    };
}

QtPromise::QPromise<AeqVersion> AeqPackageManager::getLocalVersion()
{
    return QPromise<AeqVersion>{[&](
        const QtPromise::QPromiseResolve<AeqVersion>& resolve,
                const QtPromise::QPromiseReject<AeqVersion>& reject) {
            QFile versionJson = (databaseDirectory() + "/version.json");
            if(!versionJson.exists())
            {
                reject();
            }

            versionJson.open(QFile::ReadOnly);
            QJsonDocument d = QJsonDocument::fromJson(versionJson.readAll());
            QJsonArray root = d.array();
            if(root.count() > 0)
            {
                versionJson.close();
                resolve(AeqVersion(root[0].toObject()));
            }

            versionJson.close();
            reject();
        }
    };
}

QtPromise::QPromise<QVector<AeqMeasurement>> AeqPackageManager::getLocalIndex()
{
    return QPromise<QVector<AeqMeasurement>>{[&](
        const QtPromise::QPromiseResolve<QVector<AeqMeasurement>>& resolve,
                const QtPromise::QPromiseReject<QVector<AeqMeasurement>>& reject) {
            QFile indexJson = (databaseDirectory() + "/index.json");
            if(!indexJson.exists())
            {
                reject();
            }

            indexJson.open(QFile::ReadOnly);
            QJsonDocument d = QJsonDocument::fromJson(indexJson.readAll());
            QJsonArray root = d.array();

            QVector<AeqMeasurement> items;
            for(const auto& item : root)
            {
                items.append(AeqMeasurement(item.toObject()));
            }

            indexJson.close();
            resolve(std::move(items));
        }
    };
}

QString AeqPackageManager::databaseDirectory()
{
    return AppConfig::instance().getCachePath("autoeq");
}
