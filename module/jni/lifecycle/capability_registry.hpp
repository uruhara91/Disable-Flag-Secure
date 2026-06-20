#pragma once

#include <atomic>
#include <cstdint>

namespace zsc::lifecycle {

enum Feature : uint32_t {
    kCaptureDisplay = 1u << 0,
    kCaptureLayers = 1u << 1,
    kModernScreenCaptureProfile = 1u << 2,
    kLegacySurfaceControlProfile = 1u << 3,
};

class CapabilityRegistry final {
public:
    void Replace(uint32_t value) noexcept;
    uint32_t Get() const noexcept;

private:
    std::atomic<uint32_t> value_{0};
};

CapabilityRegistry& Capabilities() noexcept;

}  // namespace zsc::lifecycle
