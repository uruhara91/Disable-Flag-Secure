#pragma once

#include <cstdint>

namespace zygisk {
struct Api;
}

namespace zsc::config {

constexpr uint32_t kConfigMagic = 0x5A534331u;  // "ZSC1"
constexpr uint16_t kConfigVersion = 1;

struct RuntimeConfig final {
    uint32_t magic;
    uint16_t version;
    uint8_t capture_secure_layers;
    uint8_t debug;
};

RuntimeConfig DefaultConfig() noexcept;
RuntimeConfig LoadFromCompanion(zygisk::Api* api) noexcept;
void CompanionHandler(int client) noexcept;

}  // namespace zsc::config
