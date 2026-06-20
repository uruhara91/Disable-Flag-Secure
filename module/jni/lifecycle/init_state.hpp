#pragma once
#include <stdint.h>
namespace zsc::lifecycle {
enum class InitState : uint32_t { kNotStarted=0, kConfigUnavailable, kDisabled, kReadyToInstall, kInstalling, kReady, kDegraded };
void SetInitState(InitState state) noexcept;
InitState GetInitState() noexcept;
}
