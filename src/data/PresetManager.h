#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include "PresetRule.h"

#include <QObject>

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

    QVector<PresetRule> rules() const;

    void setRules(const QVector<PresetRule> &newRules);

    PresetListModel *presetModel() const;

signals:
    void presetAutoloaded(const QString& deviceName);

public slots:
    void save(const QString &filename);
    void load(const QString &filename);

    void onOutputDeviceChanged(const QString& deviceName, const QString& deviceId);

    void loadRules();
    void saveRules() const;
private:
    QVector<PresetRule> _rules;
    PresetListModel* _presetModel;

    QString rulesPath() const;
};

#endif // PRESETMANAGER_H
