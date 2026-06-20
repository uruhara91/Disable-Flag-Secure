#include "process_policy.hpp"

namespace zsc::lifecycle {

bool KeepLibraryInAppProcess(JNIEnv* env, jstring nice_name) noexcept {
    (void)env;
    (void)nice_name;
    return false;
}

}  // namespace zsc::lifecycle
