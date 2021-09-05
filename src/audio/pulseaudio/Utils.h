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

template<typename T>
inline int normalize_signed_to_int(T in) {
    if(in > INT_MAX)
        return INT_MAX;
    else if(in < INT_MIN)
        return INT_MIN;
    return in;
};

template<typename T>
inline int normalize_unsigned_to_int(T in) {
    if(in <= 0)
        return 0;
    else if(in > INT_MAX)
        return INT_MAX;
    return in;
};

int random_number(int max);

inline std::string tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(std::move(item));
    }
    return elems;
}

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
