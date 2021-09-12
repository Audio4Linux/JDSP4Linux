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

auto util::linear_to_db(const float& amp) -> float {
  if (amp >= minimum_linear_level) {
    return 20.0F * std::log10(amp);
  }

  return minimum_db_level;
}

auto util::linear_to_db(const double& amp) -> double {
  if (amp >= minimum_linear_d_level) {
    return 20.0 * std::log10(amp);
  }

  return minimum_db_d_level;
}

auto util::db_to_linear(const float& db) -> float {
  return std::exp((db / 20.0F) * std::log(10.0F));
}

auto util::db_to_linear(const double& db) -> double {
  return std::exp((db / 20.0) * std::log(10.0));
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
