#include <cassert>
#include <cstring>

#include "common/hash.hpp"

int main() {
    constexpr char package_name[] = "com.android.systemui";
    constexpr uint64_t compile_time =
            zsc::common::HashAscii(package_name, sizeof(package_name) - 1);
    const uint64_t runtime =
            zsc::common::HashAscii(package_name, std::strlen(package_name));
    static_assert(compile_time != 0);
    assert(compile_time == runtime);
    assert(runtime != zsc::common::HashAscii("com.miui.screenshot", 19));
    return 0;
}
