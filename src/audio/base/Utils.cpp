#include "Utils.h"

#include <random>
#include <utility>
#include <filesystem>
#include <glib-object.h>
#include <glib.h>
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


void util::debug(const std::string& s, source_location location) {
  Log::debug(QString::fromStdString(s), location);
}

void util::error(const std::string& s, source_location location) {
  Log::error(QString::fromStdString(s), location);
}

void util::critical(const std::string& s, source_location location) {
  Log::critical(QString::fromStdString(s), location);
}

void util::warning(const std::string& s, source_location location) {
  Log::warning(QString::fromStdString(s), location);
}

void util::info(const std::string& s, source_location location) {
  Log::information(QString::fromStdString(s), location);
}

void util::idle_add(std::function<void()> cb) {
  struct Data {
    std::function<void()> cb;
  };

  auto* d = new Data();

  d->cb = std::move(cb);

  g_idle_add((GSourceFunc) +
                 [](Data* d) {
                   if (d == nullptr) {
                     return G_SOURCE_REMOVE;
                   }

                   if (d->cb == nullptr) {
                     return G_SOURCE_REMOVE;
                   }

                   d->cb();

                   delete d;

                   return G_SOURCE_REMOVE;
                 },
             d);
}

void generate_tags(const int& N, const std::string& start_string, const std::string& end_string) {
  auto max_tag_size = 0U;
  std::string body = "{";
  std::string msg = "constexpr char tag_array[][";

  for (int n = 0; n < N; n++) {
    auto n_str = util::to_string(n);

    auto tag = "\"" + start_string + n_str + end_string + "\"";

    body += "{" + tag + "}";

    if (n < N - 1) {
      body += ", ";
    }

    max_tag_size = (tag.size() > max_tag_size) ? tag.size() : max_tag_size;
  }

  msg += util::to_string(max_tag_size) + "] = " + body + "};";

  util::warning(msg);
}

auto get_files_name(const std::filesystem::path& dir_path, const std::string& ext) -> std::vector<std::string> {
  std::vector<std::string> names;

  for (std::filesystem::directory_iterator it{dir_path}; it != std::filesystem::directory_iterator{}; ++it) {
    if (std::filesystem::is_regular_file(it->status())) {
      if (it->path().extension() == ext) {
        names.push_back(it->path().stem().string());
      }
    }
  }

  return names;
}

auto util::str_contains(const std::string& haystack, const std::string& needle) -> bool {
  // This helper indicates if the needle is contained in the haystack string,
  // but the empty needle will NOT return true.

  // Instead .find method of C++ string class returns a size_type different
  // than std::string::npos when the needle is empty indicating that an empty
  // string IS CONTAINED in the haystack. That's pointless, so here is this helper.

  if (needle.empty()) {
    return false;
  }

  return (haystack.find(needle) != std::string::npos);
}

