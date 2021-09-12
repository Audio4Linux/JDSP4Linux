#ifndef CUSTOMCOOKIEJAR_H
#define CUSTOMCOOKIEJAR_H

#include <QObject>
#include <QNetworkCookieJar>
#include <QNetworkCookie>

class CustomCookieJar : public QNetworkCookieJar
{
public:
    CustomCookieJar();
    QList<QNetworkCookie> getCookies() const;
};

#endif // CUSTOMCOOKIEJAR_H
