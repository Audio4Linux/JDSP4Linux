#ifndef THROTTLEDHTTP_H
#define THROTTLEDHTTP_H

#include "http.h"
#include <QtCore>
#include <QtNetwork>

class ThrottledHttp : public Http {
public:
    ThrottledHttp(Http &http = Http::instance());
    void setMilliseconds(int milliseconds) { this->milliseconds = milliseconds; }
    HttpReply *request(const HttpRequest &req);

private:
    Http &http;
    int milliseconds;
    QElapsedTimer elapsedTimer;
};

class ThrottledHttpReply : public HttpReply {
    Q_OBJECT

public:
    ThrottledHttpReply(Http &http,
                       const HttpRequest &req,
                       int milliseconds,
                       QElapsedTimer &elapsedTimer);
    QUrl url() const { return req.url; }
    int statusCode() const { return 200; }
    QByteArray body() const { return QByteArray(); }

private slots:
    void checkElapsed();

private:
    void doRequest();
    Http &http;
    HttpRequest req;
    int milliseconds;
    QElapsedTimer &elapsedTimer;
    QTimer *timer;
};

#endif // THROTTLEDHTTP_H
