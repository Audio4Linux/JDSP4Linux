#ifndef AEQPACKAGEMANAGER_H
#define AEQPACKAGEMANAGER_H

#include <QtPromise>
#include <QObject>
#include "AeqStructs.h"

class AeqPackageManager : public QObject
{
    Q_OBJECT
public:
    explicit AeqPackageManager(QObject *parent = nullptr);

    QtPromise::QPromise<void> installPackage(AeqVersion version);
    bool uninstallPackage();

    bool isPackageInstalled();

    QString targetDirectory();

    QtPromise::QPromise<AeqVersion> getRepositoryVersion();

signals:

};

#endif // AEQPACKAGEMANAGER_H
