#ifndef EELVARIABLE_H
#define EELVARIABLE_H

#include <string>
#include <variant>

typedef struct {
    std::string name;
    std::variant<std::string, float> value;
    bool isString;
} EelVariable;

#endif // EELVARIABLE_H
