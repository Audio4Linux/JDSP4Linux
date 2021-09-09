#include "Utils.h"

#include <random>
#include "utils/Log.h"

int util::random_number(int max)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(0, max);

    return dist(mt);
}

void util::debug(const std::string &s) {
    Log::debug(QString::fromStdString(s));
}

void util::error(const std::string &s) {
    Log::error(QString::fromStdString(s));
}

void util::critical(const std::string &s) {
    Log::critical(QString::fromStdString(s));
}

void util::warning(const std::string &s) {
    Log::debug(QString::fromStdString(s));
}

void util::info(const std::string &s) {
    Log::information(QString::fromStdString(s));
}
