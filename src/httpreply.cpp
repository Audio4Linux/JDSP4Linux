#include "httpreply.h"

HttpReply::HttpReply(QObject *parent) : QObject(parent) {}

int HttpReply::isSuccessful() const {
    return statusCode() >= 200 && statusCode() < 300;
}

QString HttpReply::reasonPhrase() const {
    return QString();
}

const QList<QNetworkReply::RawHeaderPair> HttpReply::headers() const {
    return QList<QNetworkReply::RawHeaderPair>();
}

QByteArray HttpReply::header(const QByteArray &headerName) const {
    Q_UNUSED(headerName);
    return QByteArray();
}
