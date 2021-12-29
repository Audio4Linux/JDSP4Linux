#ifndef CRASHREPORTSENDER_H
#define CRASHREPORTSENDER_H

#include <qtpromise/qpromise.h>

class CrashReportSender
{
public:
    static QtPromise::QPromise<void> upload(const QString& logPath, const QString& dumpPath);

};

#endif // CRASHREPORTSENDER_H
