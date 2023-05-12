#include "SingleInstanceMonitor.h"

#include "Log.h"
#include "dbus/ServerAdaptor.h"
#include "dbus/ClientProxy.h"

SingleInstanceMonitor::SingleInstanceMonitor(QObject* parent) : QObject(parent)
{
    QDBusConnection connection = QDBusConnection::sessionBus();

    _dbusAdapter = new GuiAdaptor(this);
    _registered  = connection.registerObject("/jdsp4linux/gui", this) &&
            connection.registerService("me.timschneeberger.jdsp4linux");

}

SingleInstanceMonitor::~SingleInstanceMonitor()
{
    _dbusAdapter->deleteLater();
}

bool SingleInstanceMonitor::isServiceReady()
{
    if (_registered)
    {
        Log::information("Service registration successful");
        return true;
    }
    else
    {
        Log::warning("Service registration failed. Name already aquired by other instance");
        return false;
    }
}

bool SingleInstanceMonitor::handover()
{
    Log::information("Attempting to switch to this instance...");
    auto m_dbInterface = new me::timschneeberger::jdsp4linux::Gui("me.timschneeberger.jdsp4linux", "/jdsp4linux/gui",
                                                          QDBusConnection::sessionBus(), this);
    if (!m_dbInterface->isValid())
    {
        Log::error("Unable to connect to other DBus instance. Continuing anyway...");
    }
    else
    {
        QDBusPendingReply<> msg = m_dbInterface->raiseWindow();

        if (msg.isError() || msg.isValid())
        {
            Log::error("Other instance returned (invalid) error message. Continuing anyway...");
        }
        else
        {
            Log::information("Success! Waiting for event loop to exit...");
            Log::console("\n" + tr("Another instance of JamesDSP is already active and has been put in the foreground."), true);

            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
            return true;
        }
    }

    return false;
}
