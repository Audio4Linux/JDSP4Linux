#ifndef AEQPACKAGEMANAGER_H
#define AEQPACKAGEMANAGER_H

#include <QtPromise>
#include <QObject>
#include <QNetworkAccessManager>

#include "AeqStructs.h"

class AeqPackageManager : public QObject
{
    Q_OBJECT
public:
    explicit AeqPackageManager(QObject *parent = nullptr);

    QtPromise::QPromise<void> installPackage(AeqVersion version, QWidget* hostWindow = nullptr);
    bool uninstallPackage();

    bool isPackageInstalled();
    QString databaseDirectory();

    QtPromise::QPromise<AeqVersion> isUpdateAvailable();
    QtPromise::QPromise<AeqVersion> getRepositoryVersion();
    QtPromise::QPromise<AeqVersion> getLocalVersion();
    QtPromise::QPromise<QVector<AeqMeasurement>> getLocalIndex();

private:
    QNetworkAccessManager* nam;

};

#endif // AEQPACKAGEMANAGER_H
