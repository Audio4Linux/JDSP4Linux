#ifndef IAUDIOSERVICE_H
#define IAUDIOSERVICE_H

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

signals:
    void eelCompilationFinished(int ret);

};

Q_DECLARE_INTERFACE(IAudioService, "IAudioService")

#endif // IAUDIOSERVICE_H
