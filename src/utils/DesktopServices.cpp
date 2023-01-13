#include "DesktopServices.h"

#include "utils/Log.h"

#include <QMessageBox>

bool DesktopServices::openUrl(const QString &url, QWidget* parent)
{
    // QDesktopServices::openUrl broken on some KDE systems with Qt 5.15
    if(system(QString("xdg-open %1").arg(url).toStdString().c_str()) > 0)
    {
        Log::warning("xdg-open failed. Trying gio instead.");
        if(system(QString("gio open %1").arg(url).toStdString().c_str()) > 0)
        {
            Log::warning("gio failed. Trying kde-open5 instead.");
            if(system(QString("kde-open5 %1").arg(url).toStdString().c_str()) > 0)
            {
                if(parent != nullptr)
                {
                    QMessageBox::critical(parent, QObject::tr("Something went wrong"),
                                          QObject::tr("Failed to open URL with default browser.\n Please copy this URL manually: ") + url);
                }
                return false;
            }
        }
    }

    return true;
}
