#ifndef SINGLEINSTANCEMONITOR_H
#define SINGLEINSTANCEMONITOR_H

#include <QObject>

class GuiAdaptor;

class SingleInstanceMonitor : public QObject
{
    Q_OBJECT
public:
    SingleInstanceMonitor(QObject* parent = nullptr);
    ~SingleInstanceMonitor();

    bool handover();
    bool isServiceReady();

signals:
    void raiseWindow();

private:
    GuiAdaptor* _dbusAdapter;
    bool _registered;
};

#endif // SINGLEINSTANCEMONITOR_H
