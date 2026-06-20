#include "jni_utils.hpp"

#include <stddef.h>
#include <string.h>

#include "hash.hpp"

namespace zsc::common {

bool ClearException(JNIEnv* env) noexcept {
    if (env == nullptr || !env->ExceptionCheck()) return false;
    env->ExceptionClear();
    return true;
}

jclass FindClassNoThrow(JNIEnv* env, const char* class_name) noexcept {
    if (env == nullptr || class_name == nullptr || env->ExceptionCheck()) return nullptr;
    jclass clazz = env->FindClass(class_name);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return clazz;
}

jfieldID GetFieldIdNoThrow(JNIEnv* env, jclass clazz, const char* name,
                           const char* signature) noexcept {
    if (env == nullptr || clazz == nullptr || name == nullptr || signature == nullptr ||
        env->ExceptionCheck()) {
        return nullptr;
    }
    jfieldID field = env->GetFieldID(clazz, name, signature);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return field;
}

jmethodID GetStaticMethodIdNoThrow(JNIEnv* env, jclass clazz, const char* name,
                                   const char* signature) noexcept {
    if (env == nullptr || clazz == nullptr || name == nullptr || signature == nullptr ||
        env->ExceptionCheck()) {
        return nullptr;
    }
    jmethodID method = env->GetStaticMethodID(clazz, name, signature);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return method;
}

ProcessHashes HashProcessName(JNIEnv* env, jstring nice_name) noexcept {
    ProcessHashes result{};
    if (env == nullptr || nice_name == nullptr || env->ExceptionCheck()) return result;

    const jsize length = env->GetStringLength(nice_name);
    if (env->ExceptionCheck() || length <= 0 || length > 512) {
        ClearException(env);
        return result;
    }

    const jchar* chars = env->GetStringChars(nice_name, nullptr);
    if (chars == nullptr || env->ExceptionCheck()) {
        ClearException(env);
        return result;
    }

    uint64_t exact = kFnvOffsetBasis;
    uint64_t base = kFnvOffsetBasis;
    bool base_done = false;
    bool valid = true;

    for (jsize i = 0; i < length; ++i) {
        const jchar ch = chars[i];
        if (ch > 0x7f) {
            valid = false;
            break;
        }
        const auto byte = static_cast<uint8_t>(ch);
        exact ^= byte;
        exact *= kFnvPrime;
        if (!base_done) {
            if (ch == u':') {
                base_done = true;
            } else {
                base ^= byte;
                base *= kFnvPrime;
            }
        }
    }

    env->ReleaseStringChars(nice_name, chars);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return result;
    }

    if (valid) {
        result.exact = exact;
        result.base = base;
        result.valid = true;
    }
    return result;
}

bool JStringEqualsAscii(JNIEnv* env, jstring value, const char* ascii) noexcept {
    if (env == nullptr || value == nullptr || ascii == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const size_t expected = strlen(ascii);
    const jsize length = env->GetStringLength(value);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (length < 0 || static_cast<size_t>(length) != expected) return false;

    const jchar* chars = env->GetStringChars(value, nullptr);
    if (chars == nullptr || env->ExceptionCheck()) {
        ClearException(env);
        return false;
    }

    bool equal = true;
    for (jsize i = 0; i < length; ++i) {
        if (chars[i] != static_cast<unsigned char>(ascii[i])) {
            equal = false;
            break;
        }
    }
    env->ReleaseStringChars(value, chars);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return equal;
}

}  // namespace zsc::common
