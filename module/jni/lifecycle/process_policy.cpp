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

ProcessDecision EvaluateAppProcess(
        JNIEnv* env, jstring nice_name, int sdk,
        const config::ConfigSnapshot& snapshot) noexcept {
    const common::ProcessHashes hashes =
            common::HashProcessName(env, nice_name);
    if (!hashes.valid ||
        config::ContainsExclude(snapshot, hashes.exact, hashes.base)) {
        return {ProcessRole::kIrrelevant, false};
    }

    if (NeedsAndroid11SystemUiHook(sdk, snapshot) &&
        hashes.base == kSystemUi) {
        return {ProcessRole::kSystemUi, true};
    }

    return {ProcessRole::kIrrelevant, false};
}

bool ShouldKeepSystemServer(
        int sdk, const config::ConfigSnapshot& snapshot) noexcept {
    return sdk >= 31 && sdk <= 36 &&
           config::HasFlag(snapshot, config::kCaptureSecureLayers);
}

}  // namespace zsc::lifecycle
