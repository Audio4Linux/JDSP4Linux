#include "PresetManager.h"

#include "config/AppConfig.h"
#include "config/DspConfig.h"
#include "model/PresetListModel.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PresetManager::PresetManager() : _presetModel(new PresetListModel(this))
{
    loadRules();
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
    this->_presetModel->rescan();
}

bool PresetManager::remove(const QString &name)
{
    auto path = AppConfig::instance().getPath("presets/") + name + ".conf";
    if (QFile::exists(path))
    {
        QFile::remove(path);
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

void PresetManager::onOutputDeviceChanged(const QString &deviceName, const QString &deviceId)
{
    for(const auto& rule : qAsConst(_rules))
    {
        if(rule.deviceId == deviceId)
        {
            loadFromPath(AppConfig::instance().getPath("presets/" + rule.preset + ".conf"));
            emit presetAutoloaded(deviceName);
            break;
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

QVector<PresetRule> PresetManager::rules() const
{
    return _rules;
}

