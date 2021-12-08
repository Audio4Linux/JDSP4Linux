#ifndef CACHEDHTTP_H
#define CACHEDHTTP_H

#include "http.h"

class LocalCache;

class CachedHttp : public Http {
public:
    CachedHttp(Http &http = Http::instance(), const char *name = "http");
    void setMaxSeconds(uint seconds);
    void setMaxSize(uint maxSize);
    void setCachePostRequests(bool value) { cachePostRequests = value; }
    void setIgnoreHostname(bool value) { ignoreHostname = value; }
    auto &getValidators() { return validators; };
    HttpReply *request(const HttpRequest &req);

private:
    QByteArray requestHash(const HttpRequest &req);

    Http &http;
    LocalCache *cache;
    bool cachePostRequests;
    bool ignoreHostname = false;

    /// Mapping is MIME -> validating function
    /// Use * as MIME to define a catch-all validator
    QMap<QByteArray, std::function<bool(const HttpReply &)>> validators;
};

class CachedHttpReply : public HttpReply {
    Q_OBJECT

public:
    CachedHttpReply(const QByteArray &body, const QUrl &url, bool autoSignals = true);
    QUrl url() const { return requestUrl; }
    int statusCode() const { return 200; }
    QByteArray body() const;

private slots:
    void emitSignals();

private:
    const QByteArray bytes;
    const QUrl requestUrl;
};

class WrappedHttpReply : public HttpReply {
    Q_OBJECT

public:
    WrappedHttpReply(CachedHttp &cachedHttp,
                     LocalCache *cache,
                     const QByteArray &key,
                     HttpReply *httpReply);
    QUrl url() const { return httpReply->url(); }
    int statusCode() const { return httpReply->statusCode(); }
    QByteArray body() const { return httpReply->body(); }

private slots:
    void originFinished(const HttpReply &reply);

private:
    CachedHttp &cachedHttp;
    LocalCache *cache;
    QByteArray key;
    HttpReply *httpReply;
};

#endif // CACHEDHTTP_H
