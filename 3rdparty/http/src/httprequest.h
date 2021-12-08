#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QtNetwork>

class HttpRequest {
public:
    HttpRequest() : operation(QNetworkAccessManager::GetOperation), offset(0) {}
    QUrl url;
    QNetworkAccessManager::Operation operation;
    QByteArray body;
    uint offset;
    QMap<QByteArray, QByteArray> headers;
};

#endif // HTTPREQUEST_H
