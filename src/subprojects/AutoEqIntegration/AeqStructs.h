#ifndef AEQSTRUCTS_H
#define AEQSTRUCTS_H

#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

#include <http.h>

class AeqVersion {
public:
    AeqVersion(QJsonObject pkg)
    {
        commit = pkg.value("commit").toString();
        commitTime = QDateTime::fromString(pkg.value("commit_time").toString(), "yyyy/MM/dd HH:mm:ss");
        commitTime.setTimeSpec(Qt::UTC);
        packageTime = QDateTime::fromString(pkg.value("package_time").toString(), "yyyy/MM/dd HH:mm:ss");
        packageTime.setTimeSpec(Qt::UTC);
        packageUrl = pkg.value("package_url").toString();
        for(const auto& item : pkg.value("type").toArray())
        {
            type.append(item.toString());
        }
    }

    QNetworkReply* toDownloadReply() const
    {
        HttpRequest req;
        req.url = packageUrl;
        return Http::instance().networkReply(req);
    }

    QString commit;
    QDateTime commitTime;
    QDateTime packageTime;
    QString packageUrl;
    QStringList type;
};

#endif // AEQSTRUCTS_H
