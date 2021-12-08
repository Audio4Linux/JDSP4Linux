#ifndef HTTP_H
#define HTTP_H

#include <QtNetwork>

#include "httpreply.h"
#include "httprequest.h"

class Http {
public:
    static Http &instance();
    static const QMap<QByteArray, QByteArray> &getDefaultRequestHeaders();
    static void setDefaultReadTimeout(int timeout);

    Http();

    void setRequestHeaders(const QMap<QByteArray, QByteArray> &headers);
    QMap<QByteArray, QByteArray> &getRequestHeaders();
    void addRequestHeader(const QByteArray &name, const QByteArray &value);

    void setReadTimeout(int timeout);
    int getReadTimeout() { return readTimeout; }

    int getMaxRetries() const;
    void setMaxRetries(int value);

    QNetworkReply *networkReply(const HttpRequest &req);
    virtual HttpReply *request(const HttpRequest &req);
    HttpReply *
    request(const QUrl &url,
            QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation,
            const QByteArray &body = QByteArray(),
            uint offset = 0);
    HttpReply *get(const QUrl &url);
    HttpReply *head(const QUrl &url);
    HttpReply *post(const QUrl &url, const QMap<QString, QString> &params);
    HttpReply *post(const QUrl &url, const QByteArray &body, const QByteArray &contentType);
    HttpReply *put(const QUrl &url, const QByteArray &body, const QByteArray &contentType);
    HttpReply *deleteResource(const QUrl &url);

private:
    QMap<QByteArray, QByteArray> requestHeaders;
    int readTimeout;
    int maxRetries;
};

#endif // HTTP_H
