#include "zygisk.hpp"
#include "common/log.hpp"
#include "config/config.hpp"
#include "lifecycle/capability_registry.hpp"
#include "lifecycle/feature_manager.hpp"
#include "lifecycle/init_state.hpp"
#include "lifecycle/process_policy.hpp"

namespace {

class SecureCaptureModule final : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        (void)args;
        if (api_ != nullptr) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        zsc::lifecycle::SetInitState(zsc::lifecycle::InitState::kNotStarted);

        if (!zsc::config::LoadFromCompanion(api_, &config_)) {
            zsc::lifecycle::SetInitState(zsc::lifecycle::InitState::kConfigUnavailable);
            if (api_ != nullptr) {
                api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            }
            ZSC_LOGE("configuration snapshot unavailable; hooks disabled");
            return;
        }

        if (!zsc::lifecycle::ShouldKeepSystemServer(config_)) {
            zsc::lifecycle::SetInitState(zsc::lifecycle::InitState::kDisabled);
            if (api_ != nullptr) {
                api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            }
            return;
        }

        retained_ = true;
        zsc::lifecycle::SetInitState(zsc::lifecycle::InitState::kReadyToInstall);
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        if (!retained_) return;

        zsc::lifecycle::SetInitState(zsc::lifecycle::InitState::kInstalling);
        const zsc::lifecycle::InstallReport report =
                zsc::lifecycle::InstallSystemServerFeatures(api_, env_, config_);
        zsc::lifecycle::StoreCapabilities(report.capabilities);

        const bool ready =
                report.capabilities.installed == report.capabilities.attempted &&
                report.capabilities.installed != 0;
        zsc::lifecycle::SetInitState(ready
                ? zsc::lifecycle::InitState::kReady
                : zsc::lifecycle::InitState::kDegraded);

        if (!ready) {
            ZSC_LOGE("initialization degraded; attempted=0x%08x installed=0x%08x "
                     "failed=0x%08x profile=%u hook_attempted=%u",
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
    bool retained_ = false;
};

}  // namespace

#pragma GCC visibility push(default)
REGISTER_ZYGISK_MODULE(SecureCaptureModule)
REGISTER_ZYGISK_COMPANION(zsc::config::CompanionHandler)
#pragma GCC visibility pop
