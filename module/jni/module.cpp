#include <android/api-level.h>

#include "zygisk.hpp"
#include "capture/android11_systemui_hook.hpp"
#include "common/jni_utils.hpp"
#include "common/log.hpp"
#include "config/config.hpp"
#include "lifecycle/capability_registry.hpp"
#include "lifecycle/feature_manager.hpp"
#include "lifecycle/init_state.hpp"
#include "lifecycle/process_policy.hpp"

namespace {

constexpr char kSystemUiProcess[] = "com.android.systemui";

class SecureCaptureModule final : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (api_ == nullptr || args == nullptr) return;

        sdk_ = android_get_device_api_level();
        if (!zsc::lifecycle::kAppFeatureBackendsCompiled || sdk_ != 30 ||
            !zsc::common::JStringEqualsAscii(
                    env_, args->nice_name, kSystemUiProcess)) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        if (!zsc::config::LoadFromCompanion(api_, &config_)) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const zsc::lifecycle::ProcessDecision decision =
                zsc::lifecycle::EvaluateAppProcess(
                        env_, args->nice_name, sdk_, config_);
        if (!decision.keep_library) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        app_role_ = decision.role;
        retained_ = true;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs* args) override {
        (void)args;
        if (!retained_ || sdk_ != 30 ||
            app_role_ != zsc::lifecycle::ProcessRole::kSystemUi) {
            return;
        }

        zsc::lifecycle::CapabilitySnapshot capabilities{
                zsc::lifecycle::kCaptureDisplay,
                0,
                0,
                static_cast<uint32_t>(
                        zsc::lifecycle::ProfileId::kAndroid11SystemUi),
        };
        const uint32_t installed =
                zsc::capture::InstallAndroid11SystemUiCaptureHook(api_, env_);
        capabilities.installed =
                installed & zsc::lifecycle::kCaptureDisplay;
        capabilities.failed =
                capabilities.attempted & ~capabilities.installed;
        zsc::lifecycle::StoreCapabilities(capabilities);

        if (capabilities.failed != 0) {
            ZSC_LOGE("Android 11 SystemUI adapter unavailable; installed=0x%08x",
                     installed);
        }
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        sdk_ = android_get_device_api_level();
        zsc::lifecycle::SetInitState(
                zsc::lifecycle::InitState::kNotStarted);

        if (!zsc::config::LoadFromCompanion(api_, &config_)) {
            zsc::lifecycle::SetInitState(
                    zsc::lifecycle::InitState::kConfigUnavailable);
            if (api_ != nullptr) {
                api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            }
            ZSC_LOGE("configuration snapshot unavailable; hooks disabled");
            return;
        }

        if (!zsc::lifecycle::ShouldKeepSystemServer(sdk_, config_)) {
            zsc::lifecycle::SetInitState(
                    zsc::lifecycle::InitState::kDisabled);
            if (api_ != nullptr) {
                api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            }
            return;
        }

        retained_ = true;
        zsc::lifecycle::SetInitState(
                zsc::lifecycle::InitState::kReadyToInstall);
    }

    void postServerSpecialize(
            const zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        if (!retained_) return;

        zsc::lifecycle::SetInitState(
                zsc::lifecycle::InitState::kInstalling);
        const zsc::lifecycle::InstallReport report =
                zsc::lifecycle::InstallSystemServerFeatures(
                        api_, env_, sdk_, config_);
        zsc::lifecycle::StoreCapabilities(report.capabilities);

        const bool ready =
                report.capabilities.installed ==
                        report.capabilities.attempted &&
                report.capabilities.installed != 0;
        zsc::lifecycle::SetInitState(ready
                ? zsc::lifecycle::InitState::kReady
                : zsc::lifecycle::InitState::kDegraded);

        if (!ready) {
            ZSC_LOGE("initialization degraded; attempted=0x%08x "
                     "installed=0x%08x failed=0x%08x profile=%u "
                     "hook_attempted=%u",
                     report.capabilities.attempted,
                     report.capabilities.installed,
                     report.capabilities.failed,
                     report.capabilities.profile,
                     report.hook_attempted ? 1u : 0u);
        }
    }

private:
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    zsc::config::ConfigSnapshot config_{};
    zsc::lifecycle::ProcessRole app_role_ =
            zsc::lifecycle::ProcessRole::kIrrelevant;
    int sdk_ = 0;
    bool retained_ = false;
};

}  // namespace

#pragma GCC visibility push(default)
REGISTER_ZYGISK_MODULE(SecureCaptureModule)
REGISTER_ZYGISK_COMPANION(zsc::config::CompanionHandler)
#pragma GCC visibility pop
