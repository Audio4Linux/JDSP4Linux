#ifndef LOCALCACHE_H
#define LOCALCACHE_H

#include <QtCore>

/**
 * @brief Not thread-safe
 */
class LocalCache {
public:
    static LocalCache *instance(const char *name);
    ~LocalCache();
    static QByteArray hash(const QByteArray &s);

    const QByteArray &getName() const { return name; }

    void setMaxSeconds(uint value) { maxSeconds = value; }
    void setMaxSize(uint value) { maxSize = value; }

    QByteArray value(const QByteArray &key);
    QByteArray possiblyStaleValue(const QByteArray &key);
    void insert(const QByteArray &key, const QByteArray &value);
    void clear();

private:
    LocalCache(const QByteArray &name);
    QString cachePath(const QByteArray &key) const;
    bool isCached(const QString &path);
    void expire();
#ifndef QT_NO_DEBUG_OUTPUT
    void debugStats();
#endif

    QByteArray name;
    QString directory;
    uint maxSeconds;
    qint64 maxSize;
    qint64 size;
    QMutex mutex;
    uint insertCount;

#ifndef QT_NO_DEBUG_OUTPUT
    uint hits;
    uint misses;
#endif
};

#endif // LOCALCACHE_H
