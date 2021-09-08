#ifndef UTIL_HPP
#define UTIL_HPP

#include <glib.h>

#define GFOREACH(type, item, list) for(GList *__glist = list; __glist && (item = (type)__glist->data, true); __glist = __glist->next)

#include <glib-object.h>
#include <glib.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace util {

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

int random_number(int max);

inline void debug(const std::string& s) {
    g_debug(s.c_str(), "%s");
}

inline void error(const std::string& s) {
    g_error(s.c_str(), "%s");
}

inline void critical(const std::string& s) {
    g_critical(s.c_str(), "%s");
}

inline void warning(const std::string& s) {
    g_warning(s.c_str(), "%s");
}

inline void info(const std::string& s) {
    g_info(s.c_str(), "%s");
}

}  // namespace util


#endif
