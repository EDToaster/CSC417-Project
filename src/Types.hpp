#pragma once

#include <cstdint>

typedef uint64_t ui64;
typedef int64_t i64;
typedef int32_t i32;
typedef uint8_t ui8;

inline double noise() {
    ui64 r = rand();
    // adjust because RAND_MAX is small and inclusive.
    if (r == RAND_MAX) r--;
    return r / double(RAND_MAX);
}
