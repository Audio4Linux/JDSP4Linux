#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <chrono>
#include <charconv>
#include <source_location>
#include <functional>

#ifdef __clang__
#include <experimental/source_location>
#endif

namespace util {

#ifdef __clang__
using source_location = std::experimental::source_location;
#else
using source_location = std::source_location;
#endif

const float minimum_db_level = -100.0F;
const double minimum_db_d_level = -100.0;
const float minimum_linear_level = 0.00001F;
const double minimum_linear_d_level = 0.00001;

/**
 * @brief concat_vectors
 * Concatenate two vectors efficiently by inserting the smaller one into the larger one
 */
template<typename T>
inline std::vector<T> concat_vectors(std::vector<T> v1, std::vector<T> v2){
    if(v1.size() > v2.size()) {
        v1.insert(v1.end(), v2.begin(), v2.end());
        return v1;
    } else {
        v2.insert(v2.end(), v1.begin(), v1.end());
        return v2;
    }
}

template<typename C, typename T>
bool contains(C&& c, T e) {
    return std::find(std::begin(c), std::end(c), e) != std::end(c);
};

template<typename K, typename V>
bool mapContainsKey(std::map<K, V>& map, K key)
{
    if (map.find(key) == map.end()) return false;
    return true;
}

auto linear_to_db(const float& amp) -> float;
auto linear_to_db(const double& amp) -> double;

auto db_to_linear(const float& db) -> float;
auto db_to_linear(const double& db) -> double;

int random_number(int max);

void debug(const std::string& s, source_location location = source_location::current());
void error(const std::string& s, source_location location = source_location::current());
void critical(const std::string& s, source_location location = source_location::current());
void warning(const std::string& s, source_location location = source_location::current());
void info(const std::string& s, source_location location = source_location::current());

void idle_add(std::function<void()> cb);

auto str_contains(const std::string& haystack, const std::string& needle) -> bool;

template <typename T>
auto str_to_num(const std::string& str, T& num) -> bool {
  // This is a more robust implementation of `std::from_chars`
  // so that we don't have to do every time with `std::from_chars_result` structure.
  // We don't care of error types, so a simple bool is returned on success/fail.
  // A left trim is performed on strings so that the conversion could success
  // even if there are leading whitespaces and/or the plus sign.

  auto first_char = str.find_first_not_of(" +\n\r\t");

  if (first_char == std::string::npos) {
    return false;
  }

  const auto result = std::from_chars(str.data() + first_char, str.data() + str.size(), num);

  return (result.ec == std::errc());
}

template <typename T>
auto to_string(const T& num, const std::string def = "0") -> std::string {
  // This is used to replace `std::to_string` as a locale independent
  // number conversion using `std::to_chars`.
  // An additional string parameter could be eventually provided with a
  // default value to return in case the conversion fails.

  // Max buffer length:
  // number of base-10 digits that can be represented by the type T without change +
  // number of base-10 digits that are necessary to uniquely represent all distinct
  // values of the type T (meaningful only for real numbers) +
  // room for other characters such as "+-e,."
  const size_t max = std::numeric_limits<T>::digits10 + std::numeric_limits<T>::max_digits10 + 10U;

  std::array<char, max> buffer;

  const auto p_init = buffer.data();

  const auto result = std::to_chars(p_init, p_init + max, num);

  return (result.ec == std::errc()) ? std::string(p_init, result.ptr - p_init) : def;
}


}  // namespace util


#endif
