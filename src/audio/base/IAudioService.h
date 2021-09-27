#ifndef IAUDIOSERVICE_H
#define IAUDIOSERVICE_H

#include "EelVariable.h"
#include "IOutputDevice.h"
#include "DspStatus.h"
#include "IAppManager.h"
#include <QObject>

class DspConfig;

class IAudioService : public QObject
{
    Q_OBJECT
public:
    virtual ~IAudioService() {}

public slots:
    virtual void update(DspConfig* config) = 0;
    virtual void reloadLiveprog() = 0;
    virtual void reloadService() = 0;

    virtual IAppManager* appManager() = 0;

    virtual std::vector<IOutputDevice> sinkDevices() = 0;
    virtual DspStatus status() = 0;

    virtual void enumerateLiveprogVariables() = 0;

signals:
    void eelCompilationStarted(const QString& scriptName);
    void eelCompilationFinished(int ret, const QString& retMsg, const QString& msg, const QString& scriptName, float initMs);
    void eelOutputReceived(const QString& output);
    void eelVariablesEnumerated(const std::list<EelVariable>& vars);

};

Q_DECLARE_INTERFACE(IAudioService, "IAudioService")

#endif // IAUDIOSERVICE_H
