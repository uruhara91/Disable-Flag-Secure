LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := zygisk_secure_capture
LOCAL_SRC_FILES := \
    module.cpp \
    lifecycle/process_policy.cpp \
    lifecycle/capability_registry.cpp \
    capture/jni_capture_hook.cpp \
    config/config.cpp
LOCAL_CPPFLAGS := -std=c++20 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fvisibility=hidden
LOCAL_LDFLAGS := -Wl,--gc-sections
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
