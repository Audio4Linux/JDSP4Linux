#include "LiveprogFileWatcher.h"

#include <QFile>
#include <QFileInfo>

LiveprogFileWatcher::LiveprogFileWatcher(QObject* parent) : QObject(parent)
{
    _timer.setSingleShot(true);
    _timer.setInterval(150);
    connect(&_watcher, &QFileSystemWatcher::fileChanged, this, [this] { _timer.start(); });
    connect(&_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { _timer.start(); });
    connect(&_timer, &QTimer::timeout, this, &LiveprogFileWatcher::checkFile);
}

void LiveprogFileWatcher::setPath(const QString& path)
{
    const QString absolute = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
    if (absolute == _path)
        return;

    _timer.stop();
    if (!_watcher.files().isEmpty())
        _watcher.removePaths(_watcher.files());
    if (!_watcher.directories().isEmpty())
        _watcher.removePaths(_watcher.directories());
    _path = absolute;
    _contents.clear();
    if (_path.isEmpty())
        return;

    const QString directory = QFileInfo(_path).absolutePath();
    if (!_watcher.addPath(directory))
        emit warning(tr("Cannot watch LiveProg directory: %1. Keep the script in an existing, accessible directory.").arg(directory));
    watchFile();

    // The normal configuration update loads this version. Only later edits reload.
    QFile file(_path);
    if (file.open(QIODevice::ReadOnly))
        _contents = file.readAll();
}

void LiveprogFileWatcher::watchFile()
{
    if (QFileInfo(_path).isFile() && !_watcher.files().contains(_path))
    {
        if (!_watcher.addPath(_path))
            emit warning(tr("Cannot watch LiveProg file: %1. Check file access; do not change the selected script to hide this failure.").arg(_path));
    }
}

void LiveprogFileWatcher::checkFile()
{
    watchFile();
    QFile file(_path);
    if (!QFileInfo(_path).isFile() || !file.open(QIODevice::ReadOnly))
    {
        emit warning(tr("LiveProg reload skipped: %1 is missing or unreadable. Restore the selected file; the current program remains loaded.").arg(_path));
        return;
    }

    const QByteArray contents = file.readAll();
    if (file.error() != QFileDevice::NoError || contents.trimmed().isEmpty())
    {
        emit warning(tr("LiveProg reload skipped: %1 is empty or incomplete. Save the complete script; the current program remains loaded.").arg(_path));
        return;
    }
    if (contents == _contents)
        return;

    _contents = contents;
    emit reloadRequested();
}
