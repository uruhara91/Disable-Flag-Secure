#pragma once

#include <stdint.h>

namespace zsc::lifecycle {

enum Feature : uint32_t {
    kCaptureDisplay = UINT32_C(1) << 0,
    kCaptureLayers = UINT32_C(1) << 1,
    kMetadataSanitizer = UINT32_C(1) << 2,
    kScreenshotDetection = UINT32_C(1) << 3,
    kRecordingDetection = UINT32_C(1) << 4,
    kLegacyRelayout = UINT32_C(1) << 5,
    kVendorAdapter = UINT32_C(1) << 6,

    // Runtime profile markers are reported alongside functional bits.
    kModernScreenCaptureProfile = UINT32_C(1) << 7,
    kLegacySurfaceControlProfile = UINT32_C(1) << 8,
    kAndroid11SystemUiProfile = UINT32_C(1) << 9,
};

enum class ProfileId : uint32_t {
    kNone = 0,
    kAndroid11SystemUi = 1,
    kSurfaceControlAndroid12To13 = 2,
    kScreenCaptureAndroid14To16 = 3,
};

struct CapabilitySnapshot final {
    uint32_t attempted;
    uint32_t installed;
    uint32_t failed;
    uint32_t profile;
};

void StoreCapabilities(const CapabilitySnapshot& snapshot) noexcept;
CapabilitySnapshot LoadCapabilities() noexcept;

}  // namespace zsc::lifecycle
