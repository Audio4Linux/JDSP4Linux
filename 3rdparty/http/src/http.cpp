#include "http.h"

#include "networkhttpreply.h"

namespace {

QNetworkAccessManager *networkAccessManager() {
    static thread_local QNetworkAccessManager *nam = [] {
        auto nam = new QNetworkAccessManager();
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
        return nam;
    }();
    return nam;
}

int defaultReadTimeout = 10000;
int defaultMaxRetries = 3;
} // namespace

Http::Http()
    : requestHeaders(getDefaultRequestHeaders()), readTimeout(defaultReadTimeout),
      maxRetries(defaultMaxRetries) {}

void Http::setRequestHeaders(const QMap<QByteArray, QByteArray> &headers) {
    requestHeaders = headers;
}

QMap<QByteArray, QByteArray> &Http::getRequestHeaders() {
    return requestHeaders;
}

void Http::addRequestHeader(const QByteArray &name, const QByteArray &value) {
    requestHeaders.insert(name, value);
}

void Http::setReadTimeout(int timeout) {
    readTimeout = timeout;
}

Http &Http::instance() {
    static Http i;
    return i;
}

const QMap<QByteArray, QByteArray> &Http::getDefaultRequestHeaders() {
    static const QMap<QByteArray, QByteArray> defaultRequestHeaders = [] {
        QMap<QByteArray, QByteArray> h;
        h.insert("Accept-Charset", "utf-8");
        h.insert("Connection", "Keep-Alive");
        return h;
    }();
    return defaultRequestHeaders;
}

void Http::setDefaultReadTimeout(int timeout) {
    defaultReadTimeout = timeout;
}

QNetworkReply *Http::networkReply(const HttpRequest &req) {
    QNetworkRequest request(req.url);

    QMap<QByteArray, QByteArray> &headers = requestHeaders;
    if (!req.headers.isEmpty()) headers = req.headers;

    QMap<QByteArray, QByteArray>::const_iterator it;
    for (it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.setRawHeader(it.key(), it.value());

    if (req.offset > 0)
        request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(req.offset).toUtf8());

    QNetworkAccessManager *manager = networkAccessManager();

    QNetworkReply *networkReply = nullptr;
    switch (req.operation) {
    case QNetworkAccessManager::GetOperation:
        networkReply = manager->get(request);
        break;

    case QNetworkAccessManager::HeadOperation:
        networkReply = manager->head(request);
        break;

    case QNetworkAccessManager::PostOperation:
        networkReply = manager->post(request, req.body);
        break;

    case QNetworkAccessManager::PutOperation:
        networkReply = manager->put(request, req.body);
        break;

    case QNetworkAccessManager::DeleteOperation:
        networkReply = manager->deleteResource(request);
        break;

    default:
        qWarning() << "Unknown operation:" << req.operation;
    }

    return networkReply;
}

HttpReply *Http::request(const HttpRequest &req) {
    return new NetworkHttpReply(req, *this);
}

HttpReply *Http::request(const QUrl &url,
                         QNetworkAccessManager::Operation operation,
                         const QByteArray &body,
                         uint offset) {
    HttpRequest req;
    req.url = url;
    req.operation = operation;
    req.body = body;
    req.offset = offset;
    return request(req);
}

HttpReply *Http::get(const QUrl &url) {
    return request(url, QNetworkAccessManager::GetOperation);
}

HttpReply *Http::head(const QUrl &url) {
    return request(url, QNetworkAccessManager::HeadOperation);
}

HttpReply *Http::post(const QUrl &url, const QMap<QString, QString> &params) {
    QByteArray body;
    QMapIterator<QString, QString> i(params);
    while (i.hasNext()) {
        i.next();
        body += QUrl::toPercentEncoding(i.key()) + '=' + QUrl::toPercentEncoding(i.value()) + '&';
    }
    HttpRequest req;
    req.url = url;
    req.operation = QNetworkAccessManager::PostOperation;
    req.body = body;
    req.headers = requestHeaders;
    req.headers.insert("Content-Type", "application/x-www-form-urlencoded");
    return request(req);
}

HttpReply *Http::post(const QUrl &url, const QByteArray &body, const QByteArray &contentType) {
    HttpRequest req;
    req.url = url;
    req.operation = QNetworkAccessManager::PostOperation;
    req.body = body;
    req.headers = requestHeaders;
    QByteArray cType = contentType;
    if (cType.isEmpty()) cType = "application/x-www-form-urlencoded";
    req.headers.insert("Content-Type", cType);
    return request(req);
}


HttpReply *Http::put(const QUrl &url, const QByteArray &body, const QByteArray &contentType) {
	HttpRequest req;
	req.url = url;
	req.operation = QNetworkAccessManager::PutOperation;
	req.body = body;
	req.headers = requestHeaders;
	QByteArray cType = contentType;
	if (cType.isEmpty()) cType = "application/x-www-form-urlencoded";
	req.headers.insert("Content-Type", cType);
	return request(req);
}


HttpReply *Http::deleteResource(const QUrl &url) {
	HttpRequest req;
	req.url = url;
	req.operation = QNetworkAccessManager::DeleteOperation;
	req.headers = requestHeaders;
	return request(req);
}

int Http::getMaxRetries() const {
    return maxRetries;
}

void Http::setMaxRetries(int value) {
    maxRetries = value;
}
