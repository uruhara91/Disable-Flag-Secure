#pragma once

#include <stddef.h>
#include <stdint.h>

namespace zsc::common {

constexpr uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr uint64_t kFnvPrime = UINT64_C(1099511628211);

constexpr uint64_t HashBytes(const void* data, size_t size,
                             uint64_t seed = kFnvOffsetBasis) noexcept {
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint64_t hash = seed;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kFnvPrime;
    }
    return hash;
}

constexpr uint64_t HashAscii(const char* text, size_t size) noexcept {
    uint64_t hash = kFnvOffsetBasis;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint8_t>(text[i]);
        hash *= kFnvPrime;
    }
    return hash;
}

}  // namespace zsc::common
