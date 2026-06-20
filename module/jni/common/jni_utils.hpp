#pragma once

#include <jni.h>
#include <stdint.h>

namespace zsc::common {

bool ClearException(JNIEnv* env) noexcept;
jclass FindClassNoThrow(JNIEnv* env, const char* class_name) noexcept;
jfieldID GetFieldIdNoThrow(JNIEnv* env, jclass clazz, const char* name,
                           const char* signature) noexcept;
jmethodID GetStaticMethodIdNoThrow(JNIEnv* env, jclass clazz, const char* name,
                                   const char* signature) noexcept;

struct ProcessHashes final {
    uint64_t exact = 0;
    uint64_t base = 0;
    bool valid = false;
};

ProcessHashes HashProcessName(JNIEnv* env, jstring nice_name) noexcept;
bool JStringEqualsAscii(JNIEnv* env, jstring value, const char* ascii) noexcept;

}  // namespace zsc::common
