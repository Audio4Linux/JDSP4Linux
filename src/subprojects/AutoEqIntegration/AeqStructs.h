#ifndef AEQSTRUCTS_H
#define AEQSTRUCTS_H

#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDir>

class AeqVersion {
public:
    AeqVersion(){}
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

    QString commit;
    QDateTime commitTime;
    QDateTime packageTime;
    QString packageUrl;
    QStringList type;
};

class AeqMeasurement {
public:
    AeqMeasurement()
    {
        name = "dummy";
        source = "dummy";
        rank = -1;
        isBest = false;
    }

    AeqMeasurement(QJsonObject pkg)
    {
        name = pkg.value("n").toString();
        source = pkg.value("s").toString();
        rank = pkg.value("r").toInt();
        isBest = rank == 1;
    }

    QString path(QString base, QString sub = "") const
    {
        return base + QDir::separator() + name + QDir::separator() + source + QDir::separator() + sub;
    }

    QString name;
    QString source;
    int rank;
    bool isBest;
};

Q_DECLARE_METATYPE(AeqVersion)
Q_DECLARE_METATYPE(AeqMeasurement)

#endif // AEQSTRUCTS_H
