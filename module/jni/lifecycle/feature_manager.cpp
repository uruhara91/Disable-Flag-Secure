#include "feature_manager.hpp"

#include "../capture/jni_capture_hook.hpp"

namespace zsc::lifecycle {

InstallReport InstallSystemServerFeatures(
        zygisk::Api* api, JNIEnv* env, int sdk,
        const config::ConfigSnapshot& snapshot) noexcept {
    InstallReport report{
            {0, 0, 0, static_cast<uint32_t>(ProfileId::kNone)},
            false,
    };

    if (config::HasFlag(snapshot, config::kCaptureSecureLayers)) {
        constexpr uint32_t desired = kCaptureDisplay | kCaptureLayers;
        report.capabilities.attempted = desired;

        const capture::CaptureInstallReport capture_report =
                capture::InstallJniCaptureHooks(api, env, sdk);
        report.hook_attempted = capture_report.hook_attempted;
        report.capabilities.installed = capture_report.installed & desired;
        report.capabilities.failed = desired & ~capture_report.installed;
        report.capabilities.profile =
                static_cast<uint32_t>(capture_report.profile);
    }

    return report;
}

}  // namespace zsc::lifecycle
