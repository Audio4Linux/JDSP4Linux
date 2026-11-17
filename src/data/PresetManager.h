#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include "PresetRule.h"

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QVector>

class PresetListModel;

class PresetManager : public QObject
{
    Q_OBJECT

public:
    static PresetManager &instance()
    {
        static PresetManager _instance;
        return _instance;
    }

    PresetManager(PresetManager const &) = delete;
    PresetManager();

    bool exists(const QString& name) const;

    QVector<PresetRule> rules() const;
    void setRules(const QVector<PresetRule> &newRules);
    bool addRule(const PresetRule& rule);
    void removeRule(const QString &deviceId, const QString &routeId);

    // Per-preset output device association (PipeWire only).
    // Lets a preset optionally remember an output device and switch to it on load.
    bool hasPresetDevice(const QString& name) const;
    void setPresetDevice(const QString& name);
    void clearPresetDevice(const QString& name);
    void applyPresetDevice(const QString& name);

    PresetListModel *presetModel() const;

signals:
    void presetAutoloaded(const QString& deviceName, const QString& routeName, bool anyRoute);
    void wantsToWriteConfig();

public slots:
    void saveToPath(const QString &filename);
    bool loadFromPath(const QString &filename);
    void rename(const QString &name, const QString &newName);
    bool remove(const QString &name);
    bool load(const QString &filename);
    void save(const QString &name);

    void onOutputDeviceChanged(const QString& deviceName, const QString& deviceId, const QString& outputRouteName);

    void loadRules();
    void saveRules() const;
private:
    QVector<PresetRule> _rules;
    QMap<QString, QJsonObject> _presetDevices;
    PresetListModel* _presetModel;
    // Suppresses applyPresetDevice() while a preset is being auto-loaded by a
    // device->preset rule, to avoid a device-change feedback loop.
    bool _suppressDeviceApply = false;

    QString rulesPath() const;
    QString presetDevicesPath() const;
    void loadPresetDevices();
    void savePresetDevices();
};

#endif // PRESETMANAGER_H
