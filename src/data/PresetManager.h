#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>

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

public slots:
    void save(const QString &filename);
    void load(const QString &filename);

};

#endif // PRESETMANAGER_H
