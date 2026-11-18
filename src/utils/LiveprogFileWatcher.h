#ifndef LIVEPROGFILEWATCHER_H
#define LIVEPROGFILEWATCHER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

// Watch the selected script and its directory: editors can replace the inode.
class LiveprogFileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit LiveprogFileWatcher(QObject* parent = nullptr);
    void setPath(const QString& path);

signals:
    void reloadRequested();
    void warning(const QString& message);

private:
    void checkFile();
    void watchFile();

    QFileSystemWatcher _watcher;
    QTimer _timer;
    QString _path;
    QByteArray _contents;
};

#endif
