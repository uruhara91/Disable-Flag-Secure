#pragma once

#include <jni.h>
#include <stdint.h>

#include "../lifecycle/capability_registry.hpp"

namespace zygisk {
struct Api;
}

namespace zsc::capture {

struct CaptureInstallReport final {
    uint32_t installed;
    lifecycle::ProfileId profile;
    bool hook_attempted;
};

CaptureInstallReport InstallJniCaptureHooks(zygisk::Api* api,
                                            JNIEnv* env) noexcept;

}  // namespace zsc::capture
