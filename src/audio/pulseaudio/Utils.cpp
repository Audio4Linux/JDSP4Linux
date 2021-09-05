#include "Utils.h"

#include <random>

int util::random_number(int max)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(0, max);

    return dist(mt);
}
