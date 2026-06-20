#include "config.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <unistd.h>

#include "../zygisk.hpp"

namespace zsc::config {
namespace {

constexpr const char* kConfigPath =
        "/data/adb/modules/zygisk_secure_capture/config/default.conf";

bool ReadFull(int fd, void* buffer, size_t size) noexcept {
    auto* out = static_cast<unsigned char*>(buffer);
    size_t done = 0;
    while (done < size) {
        const ssize_t result = read(fd, out + done, size - done);
        if (result > 0) {
            done += static_cast<size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}

bool WriteFull(int fd, const void* buffer, size_t size) noexcept {
    const auto* data = static_cast<const unsigned char*>(buffer);
    size_t done = 0;
    while (done < size) {
        const ssize_t result = write(fd, data + done, size - done);
        if (result > 0) {
            done += static_cast<size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}

char* Trim(char* value) noexcept {
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
        ++value;
    }
    char* end = value + strlen(value);
    while (end > value) {
        const char c = end[-1];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
        --end;
    }
    *end = '\0';
    return value;
}

bool ParseBoolean(const char* value, bool fallback) noexcept {
    if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "on") == 0 || strcmp(value, "1") == 0) {
        return true;
    }
    if (strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0 ||
        strcasecmp(value, "off") == 0 || strcmp(value, "0") == 0) {
        return false;
    }
    return fallback;
}

RuntimeConfig ReadConfigFile() noexcept {
    RuntimeConfig config = DefaultConfig();
    FILE* file = fopen(kConfigPath, "re");
    if (file == nullptr) {
        return config;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != nullptr) {
        char* text = Trim(line);
        if (*text == '\0' || *text == '#') {
            continue;
        }
        char* separator = strchr(text, '=');
        if (separator == nullptr) {
            continue;
        }
        *separator = '\0';
        char* key = Trim(text);
        char* value = Trim(separator + 1);

        if (strcmp(key, "capture_secure_layers") == 0) {
            config.capture_secure_layers =
                    ParseBoolean(value, config.capture_secure_layers != 0) ? 1 : 0;
        } else if (strcmp(key, "debug") == 0) {
            config.debug = ParseBoolean(value, config.debug != 0) ? 1 : 0;
        }
    }

    fclose(file);
    return config;
}

bool IsValid(const RuntimeConfig& config) noexcept {
    return config.magic == kConfigMagic && config.version == kConfigVersion;
}

}  // namespace

RuntimeConfig DefaultConfig() noexcept {
    return RuntimeConfig{kConfigMagic, kConfigVersion, 1, 0};
}

RuntimeConfig LoadFromCompanion(zygisk::Api* api) noexcept {
    RuntimeConfig config = DefaultConfig();
    if (api == nullptr) {
        return config;
    }

    const int fd = api->connectCompanion();
    if (fd < 0) {
        return config;
    }

    RuntimeConfig received{};
    const bool ok = ReadFull(fd, &received, sizeof(received));
    close(fd);
    if (ok && IsValid(received)) {
        return received;
    }
    return config;
}

void CompanionHandler(int client) noexcept {
    const RuntimeConfig config = ReadConfigFile();
    (void)WriteFull(client, &config, sizeof(config));
}

}  // namespace zsc::config
