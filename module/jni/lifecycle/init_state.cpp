#include "init_state.hpp"
namespace zsc::lifecycle { namespace { volatile uint32_t g_state=static_cast<uint32_t>(InitState::kNotStarted); }
void SetInitState(InitState state) noexcept { __atomic_store_n(&g_state,static_cast<uint32_t>(state),__ATOMIC_RELEASE); }
InitState GetInitState() noexcept { return static_cast<InitState>(__atomic_load_n(&g_state,__ATOMIC_ACQUIRE)); }
}
