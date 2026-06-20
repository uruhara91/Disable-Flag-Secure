#pragma once

#include <stddef.h>

namespace zsc::common {

bool ReadFull(int fd, void* buffer, size_t size) noexcept;
bool WriteFull(int fd, const void* buffer, size_t size) noexcept;

}  // namespace zsc::common
