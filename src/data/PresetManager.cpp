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

void PresetManager::load(const QString &filename)
{
    const QString &src  = filename;
    const QString  dest = AppConfig::instance().getDspConfPath();

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    Log::debug("PresetManager::load: Loading from " + filename);
    DspConfig::instance().load();
}

void PresetManager::save(const QString &filename)
{
    const QString  src  = AppConfig::instance().getDspConfPath();
    const QString &dest = filename;

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    Log::debug("PresetManager::save: Saving to " + filename);
}

void PresetManager::onOutputDeviceChanged(const QString &deviceName, const QString &deviceId)
{
    for(const auto& rule : qAsConst(_rules))
    {
        if(rule.deviceId == deviceId)
        {
            load(AppConfig::instance().getPath("presets/" + rule.preset + ".conf"));
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

