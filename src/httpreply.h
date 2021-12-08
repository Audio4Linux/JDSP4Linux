#ifndef HTTPREPLY_H
#define HTTPREPLY_H

#include <QtNetwork>

class HttpReply : public QObject {
    Q_OBJECT

public:
    HttpReply(QObject *parent = nullptr);
    virtual QUrl url() const = 0;
    virtual int statusCode() const = 0;
    int isSuccessful() const;
    virtual QString reasonPhrase() const;
    virtual const QList<QNetworkReply::RawHeaderPair> headers() const;
    virtual QByteArray header(const QByteArray &headerName) const;
    virtual QByteArray body() const = 0;

    template <typename Functor> HttpReply &onData(Functor lambda) {
        connect(this, &HttpReply::data, this, lambda);
        return *this;
    }
    template <typename Functor> HttpReply &onError(Functor lambda) {
        connect(this, &HttpReply::error, this, lambda);
        return *this;
    }
    template <typename Functor> HttpReply &onFinished(Functor lambda) {
        connect(this, &HttpReply::finished, this, lambda);
        return *this;
    }

signals:
    void data(const QByteArray &bytes);
    void error(const QString &message);
    void finished(const HttpReply &reply);
};

#endif // HTTPREPLY_H
