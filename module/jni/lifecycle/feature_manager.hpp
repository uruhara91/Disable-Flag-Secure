#pragma once

#include <jni.h>

#include "../config/config.hpp"
#include "capability_registry.hpp"

namespace zygisk {
struct Api;
}

namespace zsc::lifecycle {

constexpr bool kAppFeatureBackendsCompiled = false;

struct InstallReport final {
    CapabilitySnapshot capabilities;
    bool hook_attempted;
};

InstallReport InstallSystemServerFeatures(zygisk::Api* api, JNIEnv* env,
                                        const config::ConfigSnapshot& config) noexcept;

}  // namespace zsc::lifecycle
