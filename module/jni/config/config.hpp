#pragma once

#include <stddef.h>
#include <stdint.h>

namespace zygisk {
struct Api;
}

namespace zsc::config {

constexpr uint32_t kConfigMagic = UINT32_C(0x5A534332);
constexpr uint16_t kConfigVersion = 2;
constexpr size_t kMaxPackageHashes = 256;

enum FeatureFlag : uint32_t {
    kCaptureSecureLayers = UINT32_C(1) << 0,
    kMetadataSanitizer = UINT32_C(1) << 1,
    kScreenshotDetectionShield = UINT32_C(1) << 2,
    kRecordingDetectionShield = UINT32_C(1) << 3,
    kLegacyRelayoutAuto = UINT32_C(1) << 4,
    kVendorAdaptersAuto = UINT32_C(1) << 5,
    kDebugLogging = UINT32_C(1) << 31,
};

constexpr uint32_t kKnownFeatureFlags =
        kCaptureSecureLayers | kMetadataSanitizer |
        kScreenshotDetectionShield | kRecordingDetectionShield |
        kLegacyRelayoutAuto | kVendorAdaptersAuto | kDebugLogging;

// Only features with a compiled runtime backend may survive companion
// normalization. Reserved flags stay in the protocol for forward-compatible
// configuration files, but cannot retain a process or report false support.
constexpr uint32_t kCompiledFeatureFlags = kCaptureSecureLayers;
constexpr uint32_t kTargetScopedFeatureFlags =
        kScreenshotDetectionShield | kRecordingDetectionShield |
        kLegacyRelayoutAuto;

struct ConfigHeader final {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t struct_size;
    uint32_t flags;
    uint32_t target_count;
    uint32_t exclude_count;
    uint32_t reserved;
    uint64_t checksum;
};

struct ConfigSnapshot final {
    ConfigHeader header;
    uint64_t targets[kMaxPackageHashes];
    uint64_t excludes[kMaxPackageHashes];
};

void BuildSafeDisabledSnapshot(ConfigSnapshot* output) noexcept;
void BuildDefaultSnapshot(ConfigSnapshot* output) noexcept;
bool ValidateSnapshot(const ConfigSnapshot& snapshot) noexcept;
bool HasFlag(const ConfigSnapshot& snapshot, FeatureFlag flag) noexcept;
bool ContainsTarget(const ConfigSnapshot& snapshot, uint64_t exact_hash,
                    uint64_t base_hash) noexcept;
bool ContainsExclude(const ConfigSnapshot& snapshot, uint64_t exact_hash,
                     uint64_t base_hash) noexcept;
bool LoadFromCompanion(zygisk::Api* api, ConfigSnapshot* output) noexcept;
void CompanionHandler(int client) noexcept;

}  // namespace zsc::config
