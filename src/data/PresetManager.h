#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>

class PresetManager : public QObject
{
    Q_OBJECT
public:
    explicit PresetManager(QObject *parent = nullptr);

signals:

};

#endif // PRESETMANAGER_H
