#ifndef EXTRACTIONTHREAD_H
#define EXTRACTIONTHREAD_H

#include <QThread>
#include <QFile>
#include <QDir>
#include <QString>

#include "Untar.h"

class ExtractionThread : public QThread
{
    Q_OBJECT

public:
    ExtractionThread(QString _extractionPath, QString _downloadedFileName, QObject *parent = nullptr) : QThread(parent) {
        extractionPath = _extractionPath;
        downloadedFileName = _downloadedFileName;
    };
    ~ExtractionThread() {};

signals:
    void onFinished(const QString& error);

protected:
    void run() override {
        QDir destination = QDir(extractionPath);
        QFile file = QFile(downloadedFileName);

        QString errorMsg;
        destination.mkpath(destination.path());
        int ret = Untar::extract(file.fileName(), destination.path(), errorMsg);
        file.remove();

        if(ret > 0) {
            emit onFinished(errorMsg);
        }
        else {
            emit onFinished(QString());
        }
    }

private:
    QString extractionPath;
    QString downloadedFileName;
};

#endif // EXTRACTIONTHREAD_H
