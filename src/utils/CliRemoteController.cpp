#include "CliRemoteController.h"

#include "Log.h"
#include "config/DspConfig.h"
#include "data/PresetManager.h"
#include "data/model/PresetListModel.h"

#include <QVariant>

CliRemoteController::CliRemoteController(QObject *parent)
    : QObject{parent}
{
    service = new me::timschneeberger::jdsp4linux::Service("me.timschneeberger.jdsp4linux", "/jdsp4linux/service",
                                                           QDBusConnection::sessionBus(), this);
}

template<typename T>
std::optional<T> handleReply(QDBusPendingReply<T>& reply) {
    reply.waitForFinished();

    if(reply.isError()) {
        Log::error(reply.error().message());
        return {};
    }
    else
        return { reply.value() };
}

bool handleVoidReply(QDBusPendingReply<>& reply) {
    reply.waitForFinished();

    if(reply.isError()) {
        Log::error(reply.error().message());;
    }
    return reply.isError();
}

bool validateKey(const QString& keyString, DspConfig::Key& resolvedKey) {
    QMetaEnum meta = QMetaEnum::fromType<DspConfig::Key>();
    bool valid = false;
    resolvedKey = static_cast<DspConfig::Key>(meta.keyToValue(keyString.toLocal8Bit().constData(), &valid));
    if(!valid) {
        Log::error("Configuration key does not exist");
        return false;
    }
    return true;
}

bool CliRemoteController::isConnected() const
{
    return service->isValid();
}

bool CliRemoteController::set(const QString &key, const QVariant &value) const
{
    if(!checkConnectionAndLog()) {
        DspConfig::Key resolvedKey;
        bool valid = validateKey(key, resolvedKey);
        if(!valid)
            return false;

        DspConfig::instance().set(resolvedKey, value);
        return true;
    }

    auto reply = service->setAndCommit(key, QDBusVariant(value));
    return handleVoidReply(reply);
}

bool CliRemoteController::get(const QString &key) const
{
    std::optional<QString> result;
    if(!checkConnectionAndLog()) {
        DspConfig::Key resolvedKey;
        bool valid = validateKey(key, resolvedKey);
        if(!valid)
            result = std::nullopt;

        result = DspConfig::instance().get<QString>(resolvedKey);
    }
    else {
        auto reply = service->get(key);
        auto received = handleReply<QString>(reply);
        result = received;
    }

    if(result.has_value()) {
        Log::console(result.value(), true);
    }

    return result.has_value();
}

bool CliRemoteController::getAll() const
{
    std::optional<QString> result;
    if(!checkConnectionAndLog()) {
        result = DspConfig::instance().serialize();
    }
    else {
        auto reply = service->getAll();
        auto received = handleReply<QString>(reply);
        result = received;
    }

    if(result.has_value()) {
        Log::console(result.value(), true);
    }

    return result.has_value();
}

bool CliRemoteController::listKeys() const
{
    QStringList keys;
    if(checkConnectionAndLog()) {
        auto reply = service->getKeys();
        keys = handleReply(reply).value_or(keys);
    }
    keys = keys.empty() ? DspConfig::getKeys() : keys;
    Log::console(keys.join("\n"), true);
    return !keys.empty();
}

bool CliRemoteController::listPresets() const
{
    auto *model = PresetManager::instance().presetModel();
    model->rescan();
    for(const auto& name : model->getList())
        Log::console(name, true);
    return model->rowCount() > 0;
}

bool CliRemoteController::loadPreset(const QString &name) const
{
    if(checkConnectionAndLog()) {
        auto reply = service->loadPreset(name);
        return handleVoidReply(reply);
    }

    bool success = PresetManager::instance().load(name);
    if(!success)
        Log::error("Preset does not exist");
    return success;
}

bool CliRemoteController::savePreset(const QString &name) const
{
    if(checkConnectionAndLog()) {
        auto reply = service->savePreset(name);
        return handleVoidReply(reply);
    }

    PresetManager::instance().save(name);
    return true;
}

bool CliRemoteController::deletePreset(const QString &name) const
{
    if(checkConnectionAndLog()) {
        auto reply = service->deletePreset(name);
        return handleVoidReply(reply);
    }

    PresetManager::instance().remove(name);
    return true;
}

bool CliRemoteController::showStatus() const
{
    if(!requireConnectionAndLog())
        return false;

    Log::console("App version:\t" + service->appVersion(), true);
    Log::console("Core version:\t" + service->coreVersion(), true);
    Log::console("Sampling rate:\t" + service->samplingRate() + "Hz", true);
    Log::console("Audio format:\t" + service->audioFormat(), true);
    Log::console("Is processing:\t" + QString(service->isProcessing() ? "enabled" : "disabled"), true);
    return true;
}

// Helper to warn about failed connection
bool CliRemoteController::checkConnectionAndLog() const
{
    bool valid = service->isValid();
    if(!valid) Log::warning("JamesDSP service is offline; falling back to direct file access");
    return valid;
}

// Helper to alert about failed connection
bool CliRemoteController::requireConnectionAndLog() const
{
    bool valid = service->isValid();
    if(!valid) Log::error("JamesDSP service is offline. Please start it first and try again.");
    return valid;
}


