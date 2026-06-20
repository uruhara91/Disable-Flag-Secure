#include "process_policy.hpp"

#include <stddef.h>

#include "../common/hash.hpp"
#include "../common/jni_utils.hpp"

namespace zsc::lifecycle {
namespace {

template <size_t N>
constexpr uint64_t LiteralHash(const char (&value)[N]) noexcept {
    return common::HashAscii(value, N - 1);
}

constexpr uint64_t kSystemUi = LiteralHash("com.android.systemui");

bool NeedsAndroid11SystemUiHook(
        int sdk, const config::ConfigSnapshot& snapshot) noexcept {
    return sdk == 30 &&
           config::HasFlag(snapshot, config::kCaptureSecureLayers);
}

}  // namespace

ProcessDecision EvaluateAppProcess(JNIEnv* env, jstring nice_name, int sdk,
                                   const config::ConfigSnapshot& snapshot) noexcept {
    const common::ProcessHashes hashes = common::HashProcessName(env, nice_name);
    if (!hashes.valid ||
        config::ContainsExclude(snapshot, hashes.exact, hashes.base)) {
        return {ProcessRole::kIrrelevant, false};
    }

    // v0.1 compiles exactly one app-side backend: the Android 11 AOSP
    // SystemUI nativeScreenshot adapter. Future feature flags remain inert
    // until their corresponding installers are compiled.
    if (NeedsAndroid11SystemUiHook(sdk, snapshot) &&
        hashes.base == kSystemUi) {
        return {ProcessRole::kSystemUi, true};
    }

    return {ProcessRole::kIrrelevant, false};
}

bool ShouldKeepSystemServer(int sdk,
                            const config::ConfigSnapshot& snapshot) noexcept {
    const bool system_capture =
            sdk >= 31 &&
            config::HasFlag(snapshot, config::kCaptureSecureLayers);

    // Metadata sanitization is reserved but not compiled yet, so it must not
    // retain system_server merely because its configuration flag is enabled.
    return system_capture;
}

}  // namespace zsc::lifecycle
