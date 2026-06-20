#pragma once

#include <jni.h>
#include <stdint.h>

namespace zygisk {
struct Api;
}

namespace zsc::capture {

uint32_t InstallJniCaptureHooks(zygisk::Api* api, JNIEnv* env) noexcept;

}  // namespace zsc::capture
