#ifndef CRASHREPORTSENDER_H
#define CRASHREPORTSENDER_H

// CrashReportSender has been disabled. Left here in case I need to reuse parts of the code.
#if 0
#include <qtpromise/qpromise.h>

class CrashReportSender
{
public:
    static QtPromise::QPromise<void> upload(const QString& logPath, const QString& dumpPath);

};
#endif

#endif // CRASHREPORTSENDER_H
