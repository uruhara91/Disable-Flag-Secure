#include <cstdint>

#include "zygisk.hpp"
#include "capture/jni_capture_hook.hpp"
#include "common/log.hpp"
#include "config/config.hpp"
#include "lifecycle/capability_registry.hpp"
#include "lifecycle/process_policy.hpp"

namespace {

class SecureCaptureModule final : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (api_ == nullptr || args == nullptr) {
            return;
        }
        if (!zsc::lifecycle::KeepLibraryInAppProcess(env_, args->nice_name)) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        config_ = zsc::config::LoadFromCompanion(api_);
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs* args) override {
        (void)args;
        if (config_.capture_secure_layers == 0) {
            ZSC_LOGI("secure capture disabled by configuration");
            return;
        }

        const uint32_t features = zsc::capture::InstallJniCaptureHooks(api_, env_);
        zsc::lifecycle::Capabilities().Replace(features);

        if (features == 0) {
            ZSC_LOGW("no compatible AOSP capture JNI profile was found");
            return;
        }

        ZSC_LOGI("capture hooks installed; capability mask=0x%08x", features);
    }

private:
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    zsc::config::RuntimeConfig config_ = zsc::config::DefaultConfig();
};

}  // namespace

REGISTER_ZYGISK_MODULE(SecureCaptureModule)
REGISTER_ZYGISK_COMPANION(zsc::config::CompanionHandler)
