#include "feature_manager.hpp"
#include "../capture/jni_capture_hook.hpp"
namespace zsc::lifecycle {
InstallReport InstallSystemServerFeatures(zygisk::Api* api, JNIEnv* env,const config::ConfigSnapshot& snapshot) noexcept {
    InstallReport report{{0,0,0,static_cast<uint32_t>(ProfileId::kNone)},false};
    if (config::HasFlag(snapshot,config::kMetadataSanitizer)) { report.capabilities.attempted|=kMetadataSanitizer; report.capabilities.failed|=kMetadataSanitizer; }
    if (config::HasFlag(snapshot,config::kCaptureSecureLayers)) {
        report.capabilities.attempted|=kCaptureDisplay|kCaptureLayers;
        report.hook_attempted=true;
        const uint32_t installed=capture::InstallJniCaptureHooks(api,env);
        report.capabilities.installed|=installed&(kCaptureDisplay|kCaptureLayers);
        report.capabilities.failed|=(kCaptureDisplay|kCaptureLayers)&~installed;
        if ((installed&kLegacySurfaceControlProfile)!=0) report.capabilities.profile=static_cast<uint32_t>(ProfileId::kSurfaceControlAndroid12To13);
        else if ((installed&kModernScreenCaptureProfile)!=0) report.capabilities.profile=static_cast<uint32_t>(ProfileId::kScreenCaptureAndroid14To16);
    }
    return report;
}
}
