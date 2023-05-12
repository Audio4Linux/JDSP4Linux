#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include "PresetRule.h"

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

    PresetListModel *presetModel() const;

signals:
    void presetAutoloaded(const QString& deviceName);
    void wantsToWriteConfig();

public slots:
    void saveToPath(const QString &filename);
    bool loadFromPath(const QString &filename);
    void rename(const QString &name, const QString &newName);
    bool remove(const QString &name);
    bool load(const QString &filename);
    void save(const QString &name);

    void onOutputDeviceChanged(const QString& deviceName, const QString& deviceId);

    void loadRules();
    void saveRules() const;
private:
    QVector<PresetRule> _rules;
    PresetListModel* _presetModel;

    QString rulesPath() const;
};

#endif // PRESETMANAGER_H
