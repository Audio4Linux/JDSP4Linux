#ifndef IAUDIOSERVICE_H
#define IAUDIOSERVICE_H

#include "DspHost.h"
#include "EelVariable.h"
#include "IOutputDevice.h"
#include "DspStatus.h"
#include "EventArgs.h"
#include "IAppManager.h"

#include <QObject>
#include <memory>

class DspConfig;

class IAudioService : public QObject
{
    Q_OBJECT
public:
    virtual ~IAudioService() {}

public slots:
    virtual void update(DspConfig* config) = 0;

    virtual void reloadService() = 0;

    virtual IAppManager* appManager() = 0;
    virtual DspHost* host() = 0;

    virtual std::vector<IOutputDevice> sinkDevices() = 0;
    virtual DspStatus status() = 0;

    void reloadLiveprog();
    void enumerateLiveprogVariables();
    void runBenchmarks();

    void handleMessage(DspHost::Message msg, std::any arg);

signals:
    void eelCompilationStarted(const QString& scriptName);
    void eelCompilationFinished(int ret, const QString& retMsg, const QString& msg, const QString& scriptName, float initMs);
    void eelOutputReceived(const QString& output);
    void eelVariablesEnumerated(const std::vector<EelVariable>& vars);
    void convolverInfoChanged(const ConvolverInfoEventArgs& args);
    void logOutputReceived(const QString& output);
    void outputDeviceChanged(const QString& deviceName, const QString& deviceId);
    void benchmarkDone();

};

inline void IAudioService::reloadLiveprog()
{
    host()->reloadLiveprog();
}

inline void IAudioService::enumerateLiveprogVariables()
{
    auto vars = this->host()->enumEelVariables();
    emit eelVariablesEnumerated(vars);
}

inline void IAudioService::runBenchmarks()
{
    host()->runBenchmarks();
}

inline void IAudioService::handleMessage(DspHost::Message msg, std::any value)
{
    switch(msg)
    {
    case DspHost::EelCompilerResult: {
        auto args = std::any_cast<QList<QString>>(value);
        int ret = args[0].toInt();

        emit eelCompilationFinished(ret, args[4], args[1], args[2], args[3].toFloat());
        break;
    }
    case DspHost::EelCompilerStart:
        emit eelCompilationStarted(std::any_cast<QString>(value));
        break;
    case DspHost::EelWriteOutputBuffer:
        emit eelOutputReceived(std::any_cast<QString>(value));
        break;
    case DspHost::ConvolverInfoChanged:
        emit convolverInfoChanged(std::any_cast<ConvolverInfoEventArgs>(value));
        break;
    case DspHost::PrintfWriteOutputBuffer:
        emit logOutputReceived(std::any_cast<QString>(value));
        break;
    case DspHost::BenchmarkDone:
        emit benchmarkDone();
        break;
    default:
        break;
    }
}

Q_DECLARE_INTERFACE(IAudioService, "IAudioService")

#endif // IAUDIOSERVICE_H
