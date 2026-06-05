#include "PresetManager.h"

#include "config/AppConfig.h"
#include "config/DspConfig.h"
#include "model/PresetListModel.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PresetManager::PresetManager() : _presetModel(new PresetListModel(this))
{
    loadRules();
    loadPresetDevices();
}

bool PresetManager::exists(const QString &name) const
{
    return QFile::exists(AppConfig::instance().getPath("presets/") + name + ".conf");
}

bool PresetManager::loadFromPath(const QString &filename)
{
    const QString &src  = filename;
    const QString  dest = AppConfig::instance().getDspConfPath();

    if (!QFile::exists(src))
    {
        // Preset does not exist anymore, rescan presets
        this->_presetModel->rescan();
        return false;
    }

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    DspConfig::instance().load();

    // If this preset remembers an output device, switch to it. Associations belong to
    // managed presets only (those in the presets directory), keyed by file base name.
    // An external file imported via loadExternalFile() must not switch the device just
    // because its base name happens to collide with a saved preset.
    if (QFileInfo(filename).dir().absolutePath() ==
        QDir(AppConfig::instance().getPath("presets/")).absolutePath())
    {
        applyPresetDevice(QFileInfo(filename).completeBaseName());
    }

    Log::debug("Loaded " + filename);
    return true;
}


bool PresetManager::load(const QString &name)
{
    return loadFromPath(AppConfig::instance().getPath("presets/") + name + ".conf");
}

void PresetManager::rename(const QString &name, const QString &newName)
{
    auto path = AppConfig::instance().getPath("presets/") + name + ".conf";
    if (QFile::exists(path))
    {
        QFile::rename(path, QDir(path).filePath(newName + ".conf"));
    }

    // Keep the device association in sync with the preset's new name
    if (_presetDevices.contains(name))
    {
        _presetDevices[newName] = _presetDevices.take(name);
        savePresetDevices();
    }

    this->_presetModel->rescan();
}

bool PresetManager::remove(const QString &name)
{
    auto path = AppConfig::instance().getPath("presets/") + name + ".conf";
    if (QFile::exists(path))
    {
        QFile::remove(path);
        clearPresetDevice(name);
        this->_presetModel->rescan();
        return true;
    }
    return false;
}

void PresetManager::save(const QString &name)
{
    saveToPath(AppConfig::instance().getPath("presets/") + name + ".conf");
}

void PresetManager::saveToPath(const QString &filename)
{
    emit wantsToWriteConfig();

    const QString  src  = AppConfig::instance().getDspConfPath();
    const QString &dest = filename;

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    this->_presetModel->rescan();
    Log::debug("Saved to " + filename);
}

void PresetManager::onOutputDeviceChanged(const QString &deviceName, const QString &deviceId, const QString& outputRouteId)
{
    QString defaultRouteId = QString::fromStdString(RouteListModel::makeDefaultRoute().name);
    auto executeRule = [this, deviceName, outputRouteId, defaultRouteId](const PresetRule& rule){
        // A rule loads a preset *because* the device changed; do not let the loaded
        // preset re-apply a device, which would feed back into this handler.
        _suppressDeviceApply = true;
        loadFromPath(AppConfig::instance().getPath("presets/" + rule.preset + ".conf"));
        _suppressDeviceApply = false;
        emit presetAutoloaded(deviceName, rule.routeName, rule.routeId == defaultRouteId);
    };

    // Look for rule with route
    for(const auto& rule : std::as_const(_rules))
    {
        if(rule.deviceId == deviceId && rule.routeId == outputRouteId)
        {
            executeRule(rule);
            return;
        }
    }

    // Fall back to rule with wildcard route
    for(const auto& rule : std::as_const(_rules))
    {
        if(rule.deviceId == deviceId && rule.routeId == defaultRouteId)
        {
            executeRule(rule);
            return;
        }
    }
}

QString PresetManager::rulesPath() const
{
    return AppConfig::instance().getPath("preset_rules.json");
}

void PresetManager::loadRules()
{
    _rules.clear();

    QFile indexJson(rulesPath());
    if(!indexJson.exists())
    {
        return;
    }

    indexJson.open(QFile::ReadOnly);
    QJsonDocument d = QJsonDocument::fromJson(indexJson.readAll());
    QJsonArray root = d.array();

    for(const auto& item : root)
    {
        _rules.append(PresetRule(item.toObject()));
    }

    indexJson.close();
}

void PresetManager::saveRules() const
{
    QFile json(rulesPath());
    if(!json.open(QIODevice::WriteOnly)){
        Log::error("PresetRuleTableModel::save: Cannot open json file");
        return;
    }

    QJsonArray root;
    for(const auto& item : _rules)
    {
        root.append(item.toJson());
    }

    json.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    json.close();
}

PresetListModel *PresetManager::presetModel() const
{
    return _presetModel;
}

void PresetManager::setRules(const QVector<PresetRule> &newRules)
{
    _rules = newRules;
    saveRules();
}

bool PresetManager::addRule(const PresetRule &rule)
{
    // Only one rule per device & route id
    removeRule(rule.deviceId, rule.routeId);

    _rules.append(rule);
    saveRules();
    return true;
}

void PresetManager::removeRule(const QString &deviceId, const QString &routeId)
{
    for(int i = 0; i < _rules.count(); i++) {
        if(_rules[i].deviceId == deviceId && _rules[i].routeId == routeId) {
            _rules.removeAt(i);
            break;
        }
    }
    saveRules();
}

QVector<PresetRule> PresetManager::rules() const
{
    return _rules;
}

QString PresetManager::presetDevicesPath() const
{
    return AppConfig::instance().getPath("preset_devices.json");
}

void PresetManager::loadPresetDevices()
{
    _presetDevices.clear();

    QFile json(presetDevicesPath());
    if(!json.exists())
    {
        return;
    }

    json.open(QFile::ReadOnly);
    QJsonObject root = QJsonDocument::fromJson(json.readAll()).object();
    json.close();

    for(auto it = root.constBegin(); it != root.constEnd(); ++it)
    {
        _presetDevices.insert(it.key(), it.value().toObject());
    }
}

void PresetManager::savePresetDevices()
{
    QFile json(presetDevicesPath());
    if(!json.open(QIODevice::WriteOnly)){
        Log::error("PresetManager::savePresetDevices: Cannot open json file");
        return;
    }

    QJsonObject root;
    for(auto it = _presetDevices.constBegin(); it != _presetDevices.constEnd(); ++it)
    {
        root.insert(it.key(), it.value());
    }

    json.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    json.close();
}

bool PresetManager::hasPresetDevice(const QString &name) const
{
    return _presetDevices.contains(name);
}

void PresetManager::setPresetDevice(const QString &name)
{
    QJsonObject entry;
    entry["deviceId"]   = AppConfig::instance().get<QString>(AppConfig::AudioOutputDevice);
    entry["useDefault"] = AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault);
    _presetDevices[name] = entry;
    savePresetDevices();
}

void PresetManager::clearPresetDevice(const QString &name)
{
    if(_presetDevices.remove(name) > 0)
    {
        savePresetDevices();
    }
}

void PresetManager::applyPresetDevice(const QString &name)
{
    if(_suppressDeviceApply || !_presetDevices.contains(name))
    {
        return;
    }

    const QJsonObject entry = _presetDevices.value(name);

    // Mirror SettingsFragment: setting these AppConfig keys triggers a live device
    // switch via PipewireAudioService::onAppConfigUpdated.
    AppConfig::instance().set(AppConfig::AudioOutputUseDefault, entry["useDefault"].toBool());
    if(!entry["useDefault"].toBool())
    {
        AppConfig::instance().set(AppConfig::AudioOutputDevice, entry["deviceId"].toString());
    }
}

