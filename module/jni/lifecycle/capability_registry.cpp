#include "capability_registry.hpp"

namespace zsc::lifecycle {

void CapabilityRegistry::Replace(uint32_t value) noexcept {
    value_.store(value, std::memory_order_release);
}

uint32_t CapabilityRegistry::Get() const noexcept {
    return value_.load(std::memory_order_acquire);
}

CapabilityRegistry& Capabilities() noexcept {
    static CapabilityRegistry registry;
    return registry;
}

}  // namespace zsc::lifecycle
