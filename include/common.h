#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

#define container_of(ptr, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<char*>(ptr) - offsetof(type, member))

inline static uint64_t str_hash(std::string_view strView) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < strView.size(); i++) {
        h = (h + static_cast<uint8_t>(strView[i])) * 0x01000193;
    }
    return static_cast<uint64_t>(h);
}


enum class SerializationTag : uint8_t {
    NIL = 0,
    ERR = 1,
    STR = 2,
    INT = 3,
    DBL = 4,
    ARR = 5
};

