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
                    QMessageBox::critical(parent, "Something went wrong", "Failed to open URL using 'xdg-open', 'gio open', and 'kde-open5'.\n"
                                                                          "Please copy this URL manually: " + url);
                }
                return false;
            }
        }
    }

    return true;
}
