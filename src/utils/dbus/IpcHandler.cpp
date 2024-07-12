#include "IpcHandler.h"

#include "IAudioService.h"
#include "config/DspConfig.h"
#include "data/PresetManager.h"
#include "data/model/PresetListModel.h"
#include "utils/Log.h"
#include "ServerAdaptor.h"
#include "utils/VersionMacros.h"

#include <QVector>

IpcHandler::IpcHandler(IAudioService* service, QObject* parent) : QObject(parent), _service(service)
{
    _dbusAdapter = new ServiceAdaptor(this);
    _registered  = _connection.registerObject("/jdsp4linux/service", this) &&
            _connection.registerService("me.timschneeberger.jdsp4linux");

}

IpcHandler::~IpcHandler()
{
    _dbusAdapter->deleteLater();
}

bool IpcHandler::isServiceReady()
{
    if (_registered)
    {
        Log::information("Main service registration successful");
        return true;
    }
    else
    {
        Log::warning("Main service registration failed. Name already aquired by other instance");
        return false;
    }
}

QString IpcHandler::appFlavor() const
{
    return APP_FLAVOR_SHORT;
}

QString IpcHandler::appVersion() const
{
    return APP_VERSION_FULL;
}

QString IpcHandler::coreVersion() const
{
    return STR(JDSP_VERSION);
}

bool IpcHandler::isProcessing() const
{
    return _service->status().IsProcessing;
}

QString IpcHandler::samplingRate() const
{
    return QString::fromStdString(_service->status().SamplingRate);
}

QString IpcHandler::audioFormat() const
{
    return QString::fromStdString(_service->status().AudioFormat);
}

QStringList IpcHandler::getKeys() const
{
    return DspConfig::getKeys();
}

void IpcHandler::commit() const
{
    DspConfig::instance().commit();
    DspConfig::instance().save();
    emit DspConfig::instance().configBuffered();
}

QString IpcHandler::get(const QString &key) const
{
    bool exists = false;

    QVariant value = DspConfig::instance().get<QVariant>(key, &exists, false);
    if(!exists) {
        sendErrorReply(QDBusError::InvalidArgs, "Configuration key does not exist");
        return "";
    }
    return value.toString();
}

QString IpcHandler::getAll() const
{
    return DspConfig::instance().serialize();
}

void IpcHandler::set(const QString &key, const QDBusVariant &value) const
{
    setInternal(key, value);
}

void IpcHandler::setAndCommit(const QString &key, const QDBusVariant &value) const
{
    setInternal(key, value);
    commit();
}

QStringList IpcHandler::getPresets() const
{
    auto *model = PresetManager::instance().presetModel();
    model->rescan();
    return model->getList();
}

void IpcHandler::loadPreset(const QString &name) const
{
    if(!PresetManager::instance().load(name))
        sendErrorReply(QDBusError::InvalidArgs, "Preset does not exist");
}

void IpcHandler::savePreset(const QString &name) const
{
    PresetManager::instance().save(name);
}

void IpcHandler::deletePreset(const QString &name) const
{
    if(!PresetManager::instance().remove(name))
        sendErrorReply(QDBusError::InvalidArgs, "Preset does not exist");
}

void IpcHandler::setPresetRule(const QString &deviceName, const QString &deviceId, const QString &routeName, const QString &routeId, const QString &preset) const
{
#ifdef USE_PULSEAUDIO
    sendErrorReply(QDBusError::ErrorType::NotSupported, "This feature is only supported in the PipeWire version of JamesDSP");
    return;
#endif
    PresetManager::instance().addRule(PresetRule(deviceName, deviceId, deviceId, routeId, preset));
}

void IpcHandler::deletePresetRule(const QString &deviceId, const QString &routeId) const
{
#ifdef USE_PULSEAUDIO
    sendErrorReply(QDBusError::ErrorType::NotSupported, "This feature is only supported in the PipeWire version of JamesDSP");
    return;
#endif
    PresetManager::instance().removeRule(deviceId, routeId);
}

QList<PresetRule> IpcHandler::getPresetRules() const
{
#ifdef USE_PULSEAUDIO
    sendErrorReply(QDBusError::ErrorType::NotSupported, "This feature is only supported in the PipeWire version of JamesDSP");
    return QList<PresetRule>();
#endif
    return PresetManager::instance().rules().toList();
}

QList<IOutputDevice> IpcHandler::getOutputDevices() const
{
    auto devices = _service->outputDevices();
    return QVector<IOutputDevice>(devices.begin(), devices.end()).toList();
}

void IpcHandler::relinkAudioPipeline() const
{
    _service->reloadService();
    DspConfig::instance().commit();
}

void IpcHandler::setInternal(const QString &key, const QDBusVariant &value) const
{
    QMetaEnum meta = QMetaEnum::fromType<DspConfig::Key>();
    bool valid = false;
    auto resolvedKey = static_cast<DspConfig::Key>(meta.keyToValue(key.toLocal8Bit().constData(), &valid));

    if(!valid) {
        sendErrorReply(QDBusError::InvalidArgs, "Configuration key does not exist");
        return;
    }

    QVariant var = value.variant();
    int type = QtCompat::variantTypeId(var);
    // Type validation
    if(type == QMetaType::QString ||
       type == QMetaType::Bool ||
       type == QMetaType::Double ||
       var.canConvert(QMetaType::Int)) {
        DspConfig::instance().set(resolvedKey, var);
    }
    else
        sendErrorReply(QDBusError::InvalidArgs, "Invalid variant value type. Variant must contain one of these types: String, Boolean, Integer, or Float");
}

