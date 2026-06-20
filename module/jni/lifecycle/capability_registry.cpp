#include "capability_registry.hpp"
namespace zsc::lifecycle { namespace { volatile uint32_t g_attempted=0,g_installed=0,g_failed=0,g_profile=0; }
void StoreCapabilities(const CapabilitySnapshot& s) noexcept { __atomic_store_n(&g_attempted,s.attempted,__ATOMIC_RELEASE); __atomic_store_n(&g_installed,s.installed,__ATOMIC_RELEASE); __atomic_store_n(&g_failed,s.failed,__ATOMIC_RELEASE); __atomic_store_n(&g_profile,s.profile,__ATOMIC_RELEASE); }
CapabilitySnapshot LoadCapabilities() noexcept { return {__atomic_load_n(&g_attempted,__ATOMIC_ACQUIRE),__atomic_load_n(&g_installed,__ATOMIC_ACQUIRE),__atomic_load_n(&g_failed,__ATOMIC_ACQUIRE),__atomic_load_n(&g_profile,__ATOMIC_ACQUIRE)}; }
}
