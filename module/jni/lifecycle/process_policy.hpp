#pragma once

#include <jni.h>

namespace zsc::lifecycle {

// v0.1 deliberately installs no app-side hooks. Keeping this decision in one
// policy function prevents accidental library retention as features are added.
bool KeepLibraryInAppProcess(JNIEnv* env, jstring nice_name) noexcept;

}  // namespace zsc::lifecycle
