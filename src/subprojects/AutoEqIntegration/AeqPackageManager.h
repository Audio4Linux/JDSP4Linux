#ifndef AEQPACKAGEMANAGER_H
#define AEQPACKAGEMANAGER_H

#include <QPromise>
#include <QObject>
#include "AeqStructs.h"

class AeqPackageManager : public QObject
{
    Q_OBJECT
public:
    explicit AeqPackageManager(QObject *parent = nullptr);

    bool installPackage();
    bool uninstallPackage();

    bool isPackageInstalled();

    QString targetDirectory();

    QFuture<AeqVersion> getRepositoryVersion();

signals:

};

#endif // AEQPACKAGEMANAGER_H
