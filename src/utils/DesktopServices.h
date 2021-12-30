#ifndef DESKTOPSERVICES_H
#define DESKTOPSERVICES_H

#include <QString>

class QWidget;

class DesktopServices
{
public:
    static bool openUrl(const QString& url, QWidget *parent = nullptr);
};

#endif // DESKTOPSERVICES_H
