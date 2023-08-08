#ifndef BENCHMARKWORKER_H
#define BENCHMARKWORKER_H

#include <QObject>

class BenchmarkWorker : public QObject {
    Q_OBJECT
public:
    BenchmarkWorker(QObject *parent = nullptr);

public slots:
    void process();

signals:
    void finished();

private:
    void writeCache(int num, double *data);

};

#endif // BENCHMARKWORKER_H
