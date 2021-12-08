#ifndef NETWORKHTTPREPLY_H
#define NETWORKHTTPREPLY_H

#include <QtNetwork>

#include "http.h"
#include "httpreply.h"
#include "httprequest.h"

class NetworkHttpReply : public HttpReply {
    Q_OBJECT

public:
    NetworkHttpReply(const HttpRequest &req, Http &http);
    QUrl url() const;
    int statusCode() const;
    QString reasonPhrase() const;
    const QList<QNetworkReply::RawHeaderPair> headers() const;
    QByteArray header(const QByteArray &headerName) const;
    QByteArray body() const;

private slots:
    void replyFinished();
    void replyError(QNetworkReply::NetworkError);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void readTimeout();

private:
    void setupReply();
    QString errorMessage();
    void emitError();
    void emitFinished();

    Http &http;
    HttpRequest req;
    QNetworkReply *networkReply;
    QTimer *readTimeoutTimer;
    int retryCount;
    QByteArray bytes;
};

#endif // NETWORKHTTPREPLY_H
