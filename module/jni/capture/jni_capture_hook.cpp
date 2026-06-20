#include "jni_capture_hook.hpp"

#include "../common/attributes.hpp"
#include "../common/jni_utils.hpp"
#include "../common/log.hpp"
#include "../zygisk.hpp"

namespace zsc::capture {
namespace {

struct CaptureFields final {
    jfieldID allow_protected = nullptr;
    jfieldID capture_secure_layers = nullptr;
};

enum class CaptureAbi : uint8_t {
    kLegacyObjectListener,
    kModernLongListener,
    kModernLongListenerWithSync,
};

struct CaptureProfile final {
    lifecycle::ProfileId id;
    CaptureAbi abi;
    const char* native_class;
    const char* args_class;
    const char* display_signature;
    const char* layers_signature;
};

constexpr CaptureProfile kProfiles[] = {
        {
                lifecycle::ProfileId::kScreenCaptureAndroid15To16,
                CaptureAbi::kModernLongListenerWithSync,
                "android/window/ScreenCapture",
                "android/window/ScreenCapture$CaptureArgs",
                "(Landroid/window/ScreenCapture$DisplayCaptureArgs;J)I",
                "(Landroid/window/ScreenCapture$LayerCaptureArgs;JZ)I",
        },
        {
                lifecycle::ProfileId::kScreenCaptureAndroid14,
                CaptureAbi::kModernLongListener,
                "android/window/ScreenCapture",
                "android/window/ScreenCapture$CaptureArgs",
                "(Landroid/window/ScreenCapture$DisplayCaptureArgs;J)I",
                "(Landroid/window/ScreenCapture$LayerCaptureArgs;J)I",
        },
        {
                lifecycle::ProfileId::kSurfaceControlAndroid12To13,
                CaptureAbi::kLegacyObjectListener,
                "android/view/SurfaceControl",
                "android/view/SurfaceControl$CaptureArgs",
                "(Landroid/view/SurfaceControl$DisplayCaptureArgs;"
                "Landroid/view/SurfaceControl$ScreenCaptureListener;)I",
                "(Landroid/view/SurfaceControl$LayerCaptureArgs;"
                "Landroid/view/SurfaceControl$ScreenCaptureListener;)I",
        },
};

CaptureFields g_fields{};
void* g_original_display = nullptr;
void* g_original_layers = nullptr;
uint32_t g_missing_display_log_once = 0;
uint32_t g_missing_layers_log_once = 0;

using ModernDisplayFn = jint (*)(JNIEnv*, jclass, jobject, jlong);
using ModernLayersFn = jint (*)(JNIEnv*, jclass, jobject, jlong);
using ModernLayersSyncFn = jint (*)(JNIEnv*, jclass, jobject, jlong,
                                    jboolean);
using LegacyCaptureFn = jint (*)(JNIEnv*, jclass, jobject, jobject);

ZSC_ALWAYS_INLINE void* LoadOriginal(void* const* slot) noexcept {
    return __atomic_load_n(slot, __ATOMIC_ACQUIRE);
}

void StoreOriginal(void** slot, void* function) noexcept {
    __atomic_store_n(slot, function, __ATOMIC_RELEASE);
}

ZSC_ALWAYS_INLINE bool PatchCaptureArgs(JNIEnv* env, jobject args) noexcept {
    if (ZSC_UNLIKELY(env == nullptr || args == nullptr ||
                     g_fields.allow_protected == nullptr ||
                     g_fields.capture_secure_layers == nullptr ||
                     env->ExceptionCheck())) {
        return false;
    }

    // Protected/DRM capture is disabled first. Secure-layer capture is enabled
    // only when that safety write succeeds.
    env->SetBooleanField(args, g_fields.allow_protected, JNI_FALSE);
    if (ZSC_UNLIKELY(env->ExceptionCheck())) {
        env->ExceptionClear();
        return false;
    }

    env->SetBooleanField(args, g_fields.capture_secure_layers, JNI_TRUE);
    if (ZSC_UNLIKELY(env->ExceptionCheck())) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

ZSC_HOT jint HookModernDisplay(JNIEnv* env, jclass clazz, jobject args,
                               jlong listener) {
    auto original = reinterpret_cast<ModernDisplayFn>(
            LoadOriginal(&g_original_display));
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(g_missing_display_log_once,
                      "capture display wrapper missing original pointer");
        return JNI_ERR;
    }
    (void)PatchCaptureArgs(env, args);
    return original(env, clazz, args, listener);
}

ZSC_HOT jint HookModernLayers(JNIEnv* env, jclass clazz, jobject args,
                              jlong listener) {
    auto original = reinterpret_cast<ModernLayersFn>(
            LoadOriginal(&g_original_layers));
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(g_missing_layers_log_once,
                      "capture layers wrapper missing original pointer");
        return JNI_ERR;
    }
    (void)PatchCaptureArgs(env, args);
    return original(env, clazz, args, listener);
}

ZSC_HOT jint HookModernLayersSync(JNIEnv* env, jclass clazz, jobject args,
                                  jlong listener, jboolean sync) {
    auto original = reinterpret_cast<ModernLayersSyncFn>(
            LoadOriginal(&g_original_layers));
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(g_missing_layers_log_once,
                      "capture layers sync wrapper missing original pointer");
        return JNI_ERR;
    }
    (void)PatchCaptureArgs(env, args);
    return original(env, clazz, args, listener, sync);
}

ZSC_HOT jint HookLegacyDisplay(JNIEnv* env, jclass clazz, jobject args,
                               jobject listener) {
    auto original = reinterpret_cast<LegacyCaptureFn>(
            LoadOriginal(&g_original_display));
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(g_missing_display_log_once,
                      "legacy capture display wrapper missing original pointer");
        return JNI_ERR;
    }
    (void)PatchCaptureArgs(env, args);
    return original(env, clazz, args, listener);
}

ZSC_HOT jint HookLegacyLayers(JNIEnv* env, jclass clazz, jobject args,
                              jobject listener) {
    auto original = reinterpret_cast<LegacyCaptureFn>(
            LoadOriginal(&g_original_layers));
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(g_missing_layers_log_once,
                      "legacy capture layers wrapper missing original pointer");
        return JNI_ERR;
    }
    (void)PatchCaptureArgs(env, args);
    return original(env, clazz, args, listener);
}

bool ResolveFields(JNIEnv* env, const char* class_name,
                   CaptureFields* output) noexcept {
    if (output == nullptr) return false;
    jclass clazz = common::FindClassNoThrow(env, class_name);
    if (clazz == nullptr) return false;

    CaptureFields fields{};
    fields.allow_protected = common::GetFieldIdNoThrow(
            env, clazz, "mAllowProtected", "Z");
    fields.capture_secure_layers = common::GetFieldIdNoThrow(
            env, clazz, "mCaptureSecureLayers", "Z");
    env->DeleteLocalRef(clazz);
    if (env->ExceptionCheck()) env->ExceptionClear();

    if (fields.allow_protected == nullptr ||
        fields.capture_secure_layers == nullptr) {
        return false;
    }
    *output = fields;
    return true;
}

bool HasExactNativeMethods(JNIEnv* env,
                           const CaptureProfile& profile) noexcept {
    jclass clazz = common::FindClassNoThrow(env, profile.native_class);
    if (clazz == nullptr) return false;

    const jmethodID display = common::GetStaticMethodIdNoThrow(
            env, clazz, "nativeCaptureDisplay", profile.display_signature);
    const jmethodID layers = common::GetStaticMethodIdNoThrow(
            env, clazz, "nativeCaptureLayers", profile.layers_signature);
    env->DeleteLocalRef(clazz);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return display != nullptr && layers != nullptr;
}

const CaptureProfile* ProbeProfile(JNIEnv* env,
                                   CaptureFields* fields) noexcept {
    for (const CaptureProfile& profile : kProfiles) {
        CaptureFields candidate_fields{};
        if (!ResolveFields(env, profile.args_class, &candidate_fields)) {
            continue;
        }
        if (!HasExactNativeMethods(env, profile)) continue;
        *fields = candidate_fields;
        return &profile;
    }
    return nullptr;
}

void* DisplayReplacement(CaptureAbi abi) noexcept {
    return abi == CaptureAbi::kLegacyObjectListener
            ? reinterpret_cast<void*>(HookLegacyDisplay)
            : reinterpret_cast<void*>(HookModernDisplay);
}

void* LayersReplacement(CaptureAbi abi) noexcept {
    switch (abi) {
        case CaptureAbi::kLegacyObjectListener:
            return reinterpret_cast<void*>(HookLegacyLayers);
        case CaptureAbi::kModernLongListener:
            return reinterpret_cast<void*>(HookModernLayers);
        case CaptureAbi::kModernLongListenerWithSync:
            return reinterpret_cast<void*>(HookModernLayersSync);
    }
    return nullptr;
}

bool HookOne(zygisk::Api* api, JNIEnv* env, const char* class_name,
             const char* method_name, const char* signature,
             void* replacement, void** original_slot) noexcept {
    if (api == nullptr || env == nullptr || class_name == nullptr ||
        method_name == nullptr || signature == nullptr ||
        replacement == nullptr || original_slot == nullptr) {
        return false;
    }

    JNINativeMethod method{
            const_cast<char*>(method_name),
            const_cast<char*>(signature),
            replacement,
    };
    api->hookJniNativeMethods(env, class_name, &method, 1);
    if (method.fnPtr == nullptr) return false;
    StoreOriginal(original_slot, method.fnPtr);
    return true;
}

}  // namespace

CaptureInstallReport InstallJniCaptureHooks(zygisk::Api* api,
                                            JNIEnv* env) noexcept {
    CaptureInstallReport report{
            0,
            lifecycle::ProfileId::kNone,
            false,
    };
    if (api == nullptr || env == nullptr || env->ExceptionCheck()) {
        return report;
    }

    CaptureFields fields{};
    const CaptureProfile* profile = ProbeProfile(env, &fields);
    if (profile == nullptr) return report;

    // Publish immutable field IDs before any wrapper can be entered.
    g_fields = fields;
    report.profile = profile->id;
    report.hook_attempted = true;

    if (HookOne(api, env, profile->native_class, "nativeCaptureDisplay",
                profile->display_signature, DisplayReplacement(profile->abi),
                &g_original_display)) {
        report.installed |= lifecycle::kCaptureDisplay;
    }

    if (HookOne(api, env, profile->native_class, "nativeCaptureLayers",
                profile->layers_signature, LayersReplacement(profile->abi),
                &g_original_layers)) {
        report.installed |= lifecycle::kCaptureLayers;
    }

    return report;
}

}  // namespace zsc::capture
