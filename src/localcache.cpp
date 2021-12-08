#include "localcache.h"

LocalCache *LocalCache::instance(const char *name) {
    static QMap<QByteArray, LocalCache *> instances;
    auto i = instances.constFind(QByteArray::fromRawData(name, strlen(name)));
    if (i != instances.constEnd()) return i.value();
    LocalCache *instance = new LocalCache(name);
    instances.insert(instance->getName(), instance);
    return instance;
}

LocalCache::LocalCache(const QByteArray &name)
    : name(name), maxSeconds(86400 * 30), maxSize(1024 * 1024 * 100), size(0), insertCount(0) {
    directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') +
                QLatin1String(name) + QLatin1Char('/');
#ifndef QT_NO_DEBUG_OUTPUT
    hits = 0;
    misses = 0;
#endif
}

LocalCache::~LocalCache() {
#ifndef QT_NO_DEBUG_OUTPUT
    debugStats();
#endif
}

QByteArray LocalCache::hash(const QByteArray &s) {
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(s);
    const QByteArray h = QByteArray::number(*(qlonglong *)hash.result().constData(), 36);
    static const char sep('/');
    QByteArray p;
    p.reserve(h.length() + 2);
    p.append(h.at(0));
    p.append(sep);
    p.append(h.at(1));
    p.append(sep);
    p.append(h.constData() + 2, strlen(h.constData()) - 2); // p.append(h.mid(2));
    return p;
}

bool LocalCache::isCached(const QString &path) {
    bool cached = QFile::exists(path) &&
                  (maxSeconds == 0 || QFileInfo(path).birthTime().secsTo(
                                              QDateTime::currentDateTimeUtc()) < maxSeconds);
#ifndef QT_NO_DEBUG_OUTPUT
    if (!cached) misses++;
#endif
    return cached;
}

QByteArray LocalCache::value(const QByteArray &key) {
    const QString path = cachePath(key);
    if (!isCached(path)) return QByteArray();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << file.fileName() << file.errorString();
#ifndef QT_NO_DEBUG_OUTPUT
        misses++;
#endif
        return QByteArray();
    }
#ifndef QT_NO_DEBUG_OUTPUT
    hits++;
#endif
    return file.readAll();
}

QByteArray LocalCache::possiblyStaleValue(const QByteArray &key) {
    const QString path = cachePath(key);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
#ifndef QT_NO_DEBUG_OUTPUT
        misses++;
#endif
        return QByteArray();
    }
#ifndef QT_NO_DEBUG_OUTPUT
    hits++;
#endif
    return file.readAll();
}

void LocalCache::insert(const QByteArray &key, const QByteArray &value) {
    qDebug() << "Inserting" << key;
    const QString path = cachePath(key);
    const QString parentDir = path.left(path.lastIndexOf(QLatin1Char('/')));
    if (!QFile::exists(parentDir)) {
        QDir().mkpath(parentDir);
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot create" << path;
        return;
    }
    file.write(value);
    file.close();
    if (size > 0) size += value.size();

    // expire cache every n inserts
    if (maxSize > 0 && ++insertCount % 100 == 0) {
        if (size == 0 || size > maxSize) expire();
    }
}

void LocalCache::clear() {
#ifndef QT_NO_DEBUG_OUTPUT
    hits = 0;
    misses = 0;
#endif
    size = 0;
    insertCount = 0;
    mutex.lock();
    QDir(directory).removeRecursively();
    mutex.unlock();
}

QString LocalCache::cachePath(const QByteArray &key) const {
    return directory + QLatin1String(key);
}

void LocalCache::expire() {
    if (!mutex.tryLock()) return;

    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;
    QDirIterator it(directory, filters, QDirIterator::Subdirectories);

    QMultiMap<QDateTime, QString> cacheItems;
    qint64 totalSize = 0;
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info = it.fileInfo();
        cacheItems.insert(info.birthTime(), path);
        totalSize += info.size();
        qApp->processEvents();
    }

    int removedFiles = 0;
    qint64 goal = (maxSize * 9) / 10;
    auto i = cacheItems.constBegin();
    while (i != cacheItems.constEnd()) {
        if (totalSize < goal) break;
        QString name = i.value();
        QFile file(name);
        qint64 size = file.size();
        file.remove();
        totalSize -= size;
        ++removedFiles;
        ++i;
        qApp->processEvents();
    }
#ifndef QT_NO_DEBUG_OUTPUT
    debugStats();
    if (removedFiles > 0) {
        qDebug() << "Removed:" << removedFiles << "Kept:" << cacheItems.count() - removedFiles
                 << "New Size:" << totalSize;
    }
#endif

    size = totalSize;
    mutex.unlock();
}

#ifndef QT_NO_DEBUG_OUTPUT
void LocalCache::debugStats() {
    int total = hits + misses;
    if (total > 0) {
        qDebug() << "Cache:" << name << '\n'
                 << "Inserts:" << insertCount << '\n'
                 << "Requests:" << total << '\n'
                 << "Hits:" << hits << (hits * 100) / total << "%\n"
                 << "Misses:" << misses << (misses * 100) / total << "%";
    }
}
#endif
