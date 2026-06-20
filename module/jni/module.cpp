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
 void onLoad(zygisk::Api* api, JNIEnv* env) override { api_=api; env_=env; }
 void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
  (void)args;
  if (api_!=nullptr) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
 }
 void preServerSpecialize(zygisk::ServerSpecializeArgs* args) override {
  (void)args;
  lifecycle::SetInitState(lifecycle::InitState::kNotStarted);
  if (!config::LoadFromCompanion(api_,&config_) || !lifecycle::ShouldKeepSystemServer(config_)) {
   lifecycle::SetInitState(lifecycle::InitState::kDisabled);
   if (api_!=nullptr) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
   return;
  }
  retained_=true;
  lifecycle::SetInitState(lifecycle::InitState::kReadyToInstall);
 }
 void postServerSpecialize(const zygisk::ServerSpecializeArgs* args) override {
  (void)args;
  if (!retained_) return;
  lifecycle::SetInitState(lifecycle::InitState::kInstalling);
  const lifecycle::InstallReport report=lifecycle::InstallSystemServerFeatures(api_,env_,config_);
  lifecycle::StoreCapabilities(report.capabilities);
  const bool ready=report.capabilities.installed==report.capabilities.attempted && report.capabilities.installed!=0;
  lifecycle::SetInitState(ready?lifecycle::InitState::kReady:lifecycle::InitState::kDegraded);
  if (!ready) ZSC_LOGE("initialization degraded; attempted=0x%08x installed=0x%08x failed=0x%08x",report.capabilities.attempted,report.capabilities.installed,report.capabilities.failed);
 }
private:
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 config::ConfigSnapshot config_{};
 bool retained_=false;
};
}

#pragma GCC visibility push(default)
REGISTER_ZYGISK_MODULE(SecureCaptureModule)
REGISTER_ZYGISK_COMPANION(zsc::config::CompanionHandler)
#pragma GCC visibility pop
