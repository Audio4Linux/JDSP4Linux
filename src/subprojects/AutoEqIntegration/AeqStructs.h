#ifndef AEQSTRUCTS_H
#define AEQSTRUCTS_H

#include <QDateTime>
#include <QString>

typedef struct {
    QString commit;
    QDateTime commitTime;
    QDateTime packageTime;
    QString packageUrl;
    QStringList type;
} AeqVersion;

#endif // AEQSTRUCTS_H
