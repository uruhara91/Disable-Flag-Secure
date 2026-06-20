#include "jni_capture_hook.hpp"

#include "../lifecycle/capability_registry.hpp"
#include "../zygisk.hpp"

namespace zsc::capture {
namespace {

struct CaptureFields final {
    jfieldID capture_secure_layers = nullptr;
    jfieldID allow_protected = nullptr;
};

CaptureFields g_modern_fields;
CaptureFields g_legacy_fields;

using ModernDisplayFn = jint (*)(JNIEnv*, jclass, jobject, jlong);
using ModernLayersV14Fn = jint (*)(JNIEnv*, jclass, jobject, jlong);
using ModernLayersV15Fn = jint (*)(JNIEnv*, jclass, jobject, jlong, jboolean);
using LegacyDisplayFn = jint (*)(JNIEnv*, jclass, jobject, jobject);
using LegacyLayersFn = jint (*)(JNIEnv*, jclass, jobject, jobject);

ModernDisplayFn g_modern_display = nullptr;
ModernLayersV14Fn g_modern_layers_v14 = nullptr;
ModernLayersV15Fn g_modern_layers_v15 = nullptr;
LegacyDisplayFn g_legacy_display = nullptr;
LegacyLayersFn g_legacy_layers = nullptr;

void ClearPendingException(JNIEnv* env) noexcept {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
}

bool ResolveFields(JNIEnv* env, const char* class_name, CaptureFields* fields) noexcept {
    jclass clazz = env->FindClass(class_name);
    if (clazz == nullptr) {
        ClearPendingException(env);
        return false;
    }

    fields->allow_protected = env->GetFieldID(clazz, "mAllowProtected", "Z");
    if (fields->allow_protected == nullptr || env->ExceptionCheck()) {
        ClearPendingException(env);
        env->DeleteLocalRef(clazz);
        *fields = {};
        return false;
    }

    fields->capture_secure_layers = env->GetFieldID(clazz, "mCaptureSecureLayers", "Z");
    if (fields->capture_secure_layers == nullptr || env->ExceptionCheck()) {
        ClearPendingException(env);
        env->DeleteLocalRef(clazz);
        *fields = {};
        return false;
    }

    env->DeleteLocalRef(clazz);
    return true;
}

void PatchCaptureArgs(JNIEnv* env, jobject args, const CaptureFields& fields) noexcept {
    if (args == nullptr || fields.allow_protected == nullptr ||
        fields.capture_secure_layers == nullptr) {
        return;
    }

    // Safety invariant: protected/DRM capture is disabled before secure-layer
    // capture is enabled. If this write fails, leave capture behavior unchanged.
    env->SetBooleanField(args, fields.allow_protected, JNI_FALSE);
    if (env->ExceptionCheck()) {
        ClearPendingException(env);
        return;
    }

    env->SetBooleanField(args, fields.capture_secure_layers, JNI_TRUE);
    ClearPendingException(env);
}

jint HookModernDisplay(JNIEnv* env, jclass clazz, jobject args, jlong listener) {
    PatchCaptureArgs(env, args, g_modern_fields);
    return g_modern_display != nullptr
            ? g_modern_display(env, clazz, args, listener)
            : JNI_ERR;
}

jint HookModernLayersV14(JNIEnv* env, jclass clazz, jobject args, jlong listener) {
    PatchCaptureArgs(env, args, g_modern_fields);
    return g_modern_layers_v14 != nullptr
            ? g_modern_layers_v14(env, clazz, args, listener)
            : JNI_ERR;
}

jint HookModernLayersV15(JNIEnv* env, jclass clazz, jobject args, jlong listener,
                         jboolean sync) {
    PatchCaptureArgs(env, args, g_modern_fields);
    return g_modern_layers_v15 != nullptr
            ? g_modern_layers_v15(env, clazz, args, listener, sync)
            : JNI_ERR;
}

jint HookLegacyDisplay(JNIEnv* env, jclass clazz, jobject args, jobject listener) {
    PatchCaptureArgs(env, args, g_legacy_fields);
    return g_legacy_display != nullptr
            ? g_legacy_display(env, clazz, args, listener)
            : JNI_ERR;
}

jint HookLegacyLayers(JNIEnv* env, jclass clazz, jobject args, jobject listener) {
    PatchCaptureArgs(env, args, g_legacy_fields);
    return g_legacy_layers != nullptr
            ? g_legacy_layers(env, clazz, args, listener)
            : JNI_ERR;
}

template <typename Function>
Function FunctionFromVoid(void* pointer) noexcept {
    return reinterpret_cast<Function>(pointer);
}

uint32_t InstallModernProfile(zygisk::Api* api, JNIEnv* env) noexcept {
    if (!ResolveFields(env, "android/window/ScreenCapture$CaptureArgs", &g_modern_fields)) {
        return 0;
    }

    JNINativeMethod methods[] = {
            {const_cast<char*>("nativeCaptureDisplay"),
             const_cast<char*>("(Landroid/window/ScreenCapture$DisplayCaptureArgs;J)I"),
             reinterpret_cast<void*>(HookModernDisplay)},
            {const_cast<char*>("nativeCaptureLayers"),
             const_cast<char*>("(Landroid/window/ScreenCapture$LayerCaptureArgs;J)I"),
             reinterpret_cast<void*>(HookModernLayersV14)},
            {const_cast<char*>("nativeCaptureLayers"),
             const_cast<char*>("(Landroid/window/ScreenCapture$LayerCaptureArgs;JZ)I"),
             reinterpret_cast<void*>(HookModernLayersV15)},
    };

    api->hookJniNativeMethods(env, "android/window/ScreenCapture", methods,
                              sizeof(methods) / sizeof(methods[0]));

    uint32_t features = 0;
    if (methods[0].fnPtr != nullptr) {
        g_modern_display = FunctionFromVoid<ModernDisplayFn>(methods[0].fnPtr);
        features |= lifecycle::kCaptureDisplay;
    }
    if (methods[1].fnPtr != nullptr) {
        g_modern_layers_v14 = FunctionFromVoid<ModernLayersV14Fn>(methods[1].fnPtr);
        features |= lifecycle::kCaptureLayers;
    }
    if (methods[2].fnPtr != nullptr) {
        g_modern_layers_v15 = FunctionFromVoid<ModernLayersV15Fn>(methods[2].fnPtr);
        features |= lifecycle::kCaptureLayers;
    }
    if (features != 0) {
        features |= lifecycle::kModernScreenCaptureProfile;
    }
    return features;
}

uint32_t InstallLegacyProfile(zygisk::Api* api, JNIEnv* env) noexcept {
    if (!ResolveFields(env, "android/view/SurfaceControl$CaptureArgs", &g_legacy_fields)) {
        return 0;
    }

    JNINativeMethod methods[] = {
            {const_cast<char*>("nativeCaptureDisplay"),
             const_cast<char*>("(Landroid/view/SurfaceControl$DisplayCaptureArgs;Landroid/view/SurfaceControl$ScreenCaptureListener;)I"),
             reinterpret_cast<void*>(HookLegacyDisplay)},
            {const_cast<char*>("nativeCaptureLayers"),
             const_cast<char*>("(Landroid/view/SurfaceControl$LayerCaptureArgs;Landroid/view/SurfaceControl$ScreenCaptureListener;)I"),
             reinterpret_cast<void*>(HookLegacyLayers)},
    };

    api->hookJniNativeMethods(env, "android/view/SurfaceControl", methods,
                              sizeof(methods) / sizeof(methods[0]));

    uint32_t features = 0;
    if (methods[0].fnPtr != nullptr) {
        g_legacy_display = FunctionFromVoid<LegacyDisplayFn>(methods[0].fnPtr);
        features |= lifecycle::kCaptureDisplay;
    }
    if (methods[1].fnPtr != nullptr) {
        g_legacy_layers = FunctionFromVoid<LegacyLayersFn>(methods[1].fnPtr);
        features |= lifecycle::kCaptureLayers;
    }
    if (features != 0) {
        features |= lifecycle::kLegacySurfaceControlProfile;
    }
    return features;
}

}  // namespace

uint32_t InstallJniCaptureHooks(zygisk::Api* api, JNIEnv* env) noexcept {
    if (api == nullptr || env == nullptr) {
        return 0;
    }

    uint32_t features = InstallModernProfile(api, env);
    if ((features & (lifecycle::kCaptureDisplay | lifecycle::kCaptureLayers)) == 0) {
        features |= InstallLegacyProfile(api, env);
    }
    return features;
}

}  // namespace zsc::capture
