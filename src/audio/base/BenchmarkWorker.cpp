#include "BenchmarkWorker.h"

#include "config/AppConfig.h"
#include "utils/Log.h"

extern "C" {
#include <jdsp_header.h>
}

BenchmarkWorker::BenchmarkWorker(QObject *parent) : QObject{parent} {}

void BenchmarkWorker::writeCache(int num, double* data) {
    QString result = QString::number(data[0]);

    for(int i = 1; i < MAX_BENCHMARK; i++)
        result.append(";" + QString::number(data[i]));

    AppConfig::instance().set(num == 0 ? AppConfig::BenchmarkCacheC0 : AppConfig::BenchmarkCacheC1, result);
}

void BenchmarkWorker::process() {
    Log::debug("Starting benchmark...");
    JamesDSP_Start_benchmark();

    double* c0 = new double[MAX_BENCHMARK];
    double* c1 = new double[MAX_BENCHMARK];
    JamesDSP_Save_benchmark(c0, c1);

    writeCache(0, c0);
    writeCache(1, c1);
    delete[] c0;
    delete[] c1;

    emit finished();
}
