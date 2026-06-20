#pragma once

#include <jni.h>
#include <stdint.h>

namespace zygisk {
struct Api;
}

namespace zsc::capture {

// Installs the Android 11 AOSP SystemUI screenshot adapter. Returns installed
// lifecycle::Feature bits; zero means the exact runtime contract was absent.
uint32_t InstallAndroid11SystemUiCaptureHook(zygisk::Api* api, JNIEnv* env) noexcept;

}  // namespace zsc::capture
