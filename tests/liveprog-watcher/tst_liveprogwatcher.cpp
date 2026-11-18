#include "LiveprogFileWatcher.h"
#include <QFile>
#include <QSaveFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class LiveprogWatcherTest : public QObject
{
    Q_OBJECT
    static void save(const QString& path, const QByteArray& contents, bool atomic = false)
    {
        if (atomic) {
            QSaveFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(contents), contents.size());
            QVERIFY(file.commit());
        } else {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(contents), contents.size());
        }
    }

private slots:
    void repeatedSaves_data()
    {
        QTest::addColumn<bool>("atomic");
        QTest::newRow("in-place") << false;
        QTest::newRow("atomic-replace") << true;
    }
    void repeatedSaves()
    {
        QFETCH(bool, atomic);
        QTemporaryDir dir;
        const QString path = dir.filePath("test.eel");
        save(path, "initial");
        LiveprogFileWatcher watcher;
        QSignalSpy reload(&watcher, &LiveprogFileWatcher::reloadRequested);
        watcher.setPath(path);
        for (int i = 1; i <= 3; ++i) {
            save(path, QByteArray::number(i), atomic);
            QTRY_COMPARE(reload.count(), i);
        }
    }
    void coalescesAndIgnoresUnchangedContent()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("test.eel");
        save(path, "initial");
        LiveprogFileWatcher watcher;
        QSignalSpy reload(&watcher, &LiveprogFileWatcher::reloadRequested);
        watcher.setPath(path);
        save(path, "one");
        QTest::qWait(30);
        save(path, "two");
        QTest::qWait(30);
        save(path, "three", true);
        QTRY_COMPARE(reload.count(), 1);
        save(path, "three", true);
        save(dir.filePath("other.eel"), "unrelated");
        QTest::qWait(400);
        QCOMPARE(reload.count(), 1);
    }
    void deletionEmptySaveAndRecovery()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("test.eel");
        save(path, "initial");
        LiveprogFileWatcher watcher;
        QSignalSpy reload(&watcher, &LiveprogFileWatcher::reloadRequested);
        QSignalSpy warnings(&watcher, &LiveprogFileWatcher::warning);
        watcher.setPath(path);
        QVERIFY(QFile::remove(path));
        QTRY_VERIFY(!warnings.isEmpty());
        QCOMPARE(reload.count(), 0);
        save(path, " \n");
        QTest::qWait(400);
        QCOMPARE(reload.count(), 0);
        save(path, "restored");
        QTRY_COMPARE(reload.count(), 1);
        save(path, "next");
        QTRY_COMPARE(reload.count(), 2);
    }
    void switchAndDisableCancelPendingReload()
    {
        QTemporaryDir dir;
        const QString first = dir.filePath("first.eel");
        const QString second = dir.filePath("second.eel");
        save(first, "first");
        save(second, "second");
        LiveprogFileWatcher watcher;
        QSignalSpy reload(&watcher, &LiveprogFileWatcher::reloadRequested);
        watcher.setPath(first);
        save(first, "pending");
        QTest::qWait(30);
        watcher.setPath(second);
        save(first, "old selection");
        QTest::qWait(400);
        QCOMPARE(reload.count(), 0);
        save(second, "new selection");
        QTRY_COMPARE(reload.count(), 1);
        save(second, "pending again");
        QTest::qWait(30);
        watcher.setPath({});
        QTest::qWait(400);
        QCOMPARE(reload.count(), 1);
    }
    void initiallyMissingFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("test.eel");
        LiveprogFileWatcher watcher;
        QSignalSpy reload(&watcher, &LiveprogFileWatcher::reloadRequested);
        watcher.setPath(path);
        save(path, "created");
        QTRY_COMPARE(reload.count(), 1);
    }
};

QTEST_GUILESS_MAIN(LiveprogWatcherTest)
#include "tst_liveprogwatcher.moc"
