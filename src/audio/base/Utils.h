#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <map>

namespace util {

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

void debug(const std::string& s);

void error(const std::string& s);

void critical(const std::string& s);

void warning(const std::string& s);

void info(const std::string& s);

}  // namespace util


#endif
