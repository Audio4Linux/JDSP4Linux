#ifndef IPCHANDLER_H
#define IPCHANDLER_H

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusVariant>
#include <QObject>

class ServiceAdaptor;
class IAudioService;

class IpcHandler : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    IpcHandler(IAudioService* service, QObject* parent = nullptr);
    ~IpcHandler();

    bool isServiceReady();

    Q_PROPERTY(QString AppFlavor READ appFlavor)
    QString appFlavor() const;

    Q_PROPERTY(QString AppVersion READ appVersion)
    QString appVersion() const;

    Q_PROPERTY(QString CoreVersion READ coreVersion)
    QString coreVersion() const;

    Q_PROPERTY(bool IsProcessing READ isProcessing)
    bool isProcessing() const;

    Q_PROPERTY(QString SamplingRate READ samplingRate)
    QString samplingRate() const;

    Q_PROPERTY(QString AudioFormat READ audioFormat)
    QString audioFormat() const;

public slots:
    QStringList getKeys() const;
    void commit() const;
    void set(const QString &key, const QDBusVariant &value) const;
    void setAndCommit(const QString &key, const QDBusVariant &value) const;
    QString get(const QString &key) const;
    QString getAll() const;
    QStringList getPresets() const;
    void loadPreset(const QString &name) const;
    void savePreset(const QString &name) const;
    void deletePreset(const QString &name) const;

private:
    QDBusConnection _connection = QDBusConnection::sessionBus();
    ServiceAdaptor* _dbusAdapter;
    bool _registered;

    IAudioService* _service;

    void setInternal(const QString &key, const QDBusVariant &value) const;
};

#endif // IPCHANDLER_H
