LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := zygisk_secure_capture
LOCAL_SRC_FILES := \
    module.cpp \
    common/io.cpp \
    common/jni_utils.cpp \
    config/config.cpp \
    lifecycle/init_state.cpp \
    lifecycle/capability_registry.cpp \
    lifecycle/process_policy.cpp \
    lifecycle/feature_manager.cpp \
    capture/android11_systemui_hook.cpp \
    capture/jni_capture_hook.cpp
LOCAL_CPPFLAGS := \
    -include $(LOCAL_PATH)/common/zygisk_api.hpp \
    -std=c++20 -O2 -flto=thin -DNDEBUG -D_FORTIFY_SOURCE=2 \
    -fno-exceptions -fno-rtti -fno-threadsafe-statics \
    -fno-semantic-interposition -fstack-protector-strong \
    -fvisibility=hidden -fvisibility-inlines-hidden \
    -ffunction-sections -fdata-sections \
    -Wall -Wextra -Wpedantic -Werror=return-type -Werror=format
LOCAL_LDFLAGS := \
    -flto=thin -Wl,--gc-sections -Wl,--icf=safe -Wl,--as-needed \
    -Wl,--exclude-libs,ALL -Wl,--no-undefined \
    -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
