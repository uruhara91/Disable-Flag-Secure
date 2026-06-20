#include "io.hpp"

#include <errno.h>
#include <stdint.h>
#include <unistd.h>

namespace zsc::common {

bool ReadFull(int fd, void* buffer, size_t size) noexcept {
    auto* output = static_cast<uint8_t*>(buffer);
    size_t done = 0;
    while (done < size) {
        const ssize_t result = read(fd, output + done, size - done);
        if (result > 0) {
            done += static_cast<size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

bool WriteFull(int fd, const void* buffer, size_t size) noexcept {
    const auto* input = static_cast<const uint8_t*>(buffer);
    size_t done = 0;
    while (done < size) {
        const ssize_t result = write(fd, input + done, size - done);
        if (result > 0) {
            done += static_cast<size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

}  // namespace zsc::common
