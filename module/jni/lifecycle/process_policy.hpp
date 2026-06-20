#pragma once

#include <jni.h>

#include "../config/config.hpp"

namespace zsc::lifecycle {

constexpr bool kAppFeatureBackendsCompiled = true;

enum class ProcessRole : unsigned char {
    kIrrelevant = 0,
    kSystemUi,
    kVendorScreenshotService,
    kTargetApplication,
};

struct ProcessDecision final {
    ProcessRole role;
    bool keep_library;
};

ProcessDecision EvaluateAppProcess(JNIEnv* env, jstring nice_name, int sdk,
                                   const config::ConfigSnapshot& config) noexcept;
bool ShouldKeepSystemServer(int sdk,
                            const config::ConfigSnapshot& config) noexcept;

}  // namespace zsc::lifecycle
