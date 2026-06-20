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
constexpr uint64_t kMiuiScreenshot = LiteralHash("com.miui.screenshot");
constexpr uint64_t kOplusScreenshot = LiteralHash("com.oplus.screenshot");
constexpr uint64_t kOplusPlatform = LiteralHash("com.oplus.appplatform");
constexpr uint64_t kFlymeSystemUi = LiteralHash("com.flyme.systemuiex");

bool NeedsTargetAppHooks(const config::ConfigSnapshot& snapshot) noexcept {
    return config::HasFlag(snapshot, config::kScreenshotDetectionShield) ||
           config::HasFlag(snapshot, config::kRecordingDetectionShield) ||
           config::HasFlag(snapshot, config::kLegacyRelayoutAuto);
}

bool NeedsScreenshotServiceHooks(int sdk,
                                 const config::ConfigSnapshot& snapshot) noexcept {
    const bool android11_capture =
            sdk == 30 &&
            config::HasFlag(snapshot, config::kCaptureSecureLayers);
    return android11_capture ||
           config::HasFlag(snapshot, config::kMetadataSanitizer) ||
           config::HasFlag(snapshot, config::kVendorAdaptersAuto);
}

}  // namespace

ProcessDecision EvaluateAppProcess(JNIEnv* env, jstring nice_name, int sdk,
                                   const config::ConfigSnapshot& snapshot) noexcept {
    const common::ProcessHashes hashes = common::HashProcessName(env, nice_name);
    if (!hashes.valid ||
        config::ContainsExclude(snapshot, hashes.exact, hashes.base)) {
        return {ProcessRole::kIrrelevant, false};
    }

    if (NeedsScreenshotServiceHooks(sdk, snapshot)) {
        if (hashes.base == kSystemUi) {
            return {ProcessRole::kSystemUi, true};
        }
        if (hashes.base == kMiuiScreenshot ||
            hashes.base == kOplusScreenshot ||
            hashes.base == kOplusPlatform ||
            hashes.base == kFlymeSystemUi) {
            return {ProcessRole::kVendorScreenshotService, true};
        }
    }

    if (NeedsTargetAppHooks(snapshot) &&
        config::ContainsTarget(snapshot, hashes.exact, hashes.base)) {
        return {ProcessRole::kTargetApplication, true};
    }

    return {ProcessRole::kIrrelevant, false};
}

bool ShouldKeepSystemServer(int sdk,
                            const config::ConfigSnapshot& snapshot) noexcept {
    const bool system_capture =
            sdk >= 31 &&
            config::HasFlag(snapshot, config::kCaptureSecureLayers);
    return system_capture ||
           config::HasFlag(snapshot, config::kMetadataSanitizer);
}

}  // namespace zsc::lifecycle
