#include "jni_capture_hook.hpp"

#include "../common/attributes.hpp"
#include "../common/jni_utils.hpp"
#include "../common/log.hpp"
#include "../zygisk.hpp"

namespace zsc::capture {
namespace {

static_assert(sizeof(void*) == 4 || sizeof(void*) == 8,
              "unsupported pointer width");

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

constexpr CaptureProfile kAndroid15To16Profile{
        lifecycle::ProfileId::kScreenCaptureAndroid15To16,
        CaptureAbi::kModernLongListenerWithSync,
        "android/window/ScreenCapture",
        "android/window/ScreenCapture$CaptureArgs",
        "(Landroid/window/ScreenCapture$DisplayCaptureArgs;J)I",
        "(Landroid/window/ScreenCapture$LayerCaptureArgs;JZ)I",
};

constexpr CaptureProfile kAndroid14Profile{
        lifecycle::ProfileId::kScreenCaptureAndroid14,
        CaptureAbi::kModernLongListener,
        "android/window/ScreenCapture",
        "android/window/ScreenCapture$CaptureArgs",
        "(Landroid/window/ScreenCapture$DisplayCaptureArgs;J)I",
        "(Landroid/window/ScreenCapture$LayerCaptureArgs;J)I",
};

constexpr CaptureProfile kAndroid12To13Profile{
        lifecycle::ProfileId::kSurfaceControlAndroid12To13,
        CaptureAbi::kLegacyObjectListener,
        "android/view/SurfaceControl",
        "android/view/SurfaceControl$CaptureArgs",
        "(Landroid/view/SurfaceControl$DisplayCaptureArgs;"
        "Landroid/view/SurfaceControl$ScreenCaptureListener;)I",
        "(Landroid/view/SurfaceControl$LayerCaptureArgs;"
        "Landroid/view/SurfaceControl$ScreenCaptureListener;)I",
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

bool MatchesProfile(JNIEnv* env, const CaptureProfile& profile,
                    CaptureFields* output) noexcept {
    CaptureFields fields{};
    if (!ResolveFields(env, profile.args_class, &fields) ||
        !HasExactNativeMethods(env, profile)) {
        return false;
    }
    *output = fields;
    return true;
}

const CaptureProfile* ProbeProfile(JNIEnv* env, int sdk,
                                   CaptureFields* output) noexcept {
    if (env == nullptr || output == nullptr || sdk < 31 || sdk > 36) {
        return nullptr;
    }

    // SDK is only a cheap ordering hint. Exact class/field/method probing
    // remains authoritative, preserving support for vendor backports.
    const CaptureProfile* ordered[3]{};
    if (sdk <= 33) {
        ordered[0] = &kAndroid12To13Profile;
        ordered[1] = &kAndroid14Profile;
        ordered[2] = &kAndroid15To16Profile;
    } else if (sdk == 34) {
        ordered[0] = &kAndroid14Profile;
        ordered[1] = &kAndroid15To16Profile;
        ordered[2] = &kAndroid12To13Profile;
    } else {
        ordered[0] = &kAndroid15To16Profile;
        ordered[1] = &kAndroid14Profile;
        ordered[2] = &kAndroid12To13Profile;
    }

    for (const CaptureProfile* profile : ordered) {
        if (MatchesProfile(env, *profile, output)) return profile;
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

uint32_t HookProfile(zygisk::Api* api, JNIEnv* env,
                     const CaptureProfile& profile) noexcept {
    void* display_replacement = DisplayReplacement(profile.abi);
    void* layers_replacement = LayersReplacement(profile.abi);
    if (api == nullptr || env == nullptr || display_replacement == nullptr ||
        layers_replacement == nullptr) {
        return 0;
    }

    JNINativeMethod methods[2]{
            {
                    const_cast<char*>("nativeCaptureDisplay"),
                    const_cast<char*>(profile.display_signature),
                    display_replacement,
            },
            {
                    const_cast<char*>("nativeCaptureLayers"),
                    const_cast<char*>(profile.layers_signature),
                    layers_replacement,
            },
    };

    api->hookJniNativeMethods(env, profile.native_class, methods, 2);

    uint32_t installed = 0;
    if (methods[0].fnPtr != nullptr) {
        StoreOriginal(&g_original_display, methods[0].fnPtr);
        installed |= lifecycle::kCaptureDisplay;
    }
    if (methods[1].fnPtr != nullptr) {
        StoreOriginal(&g_original_layers, methods[1].fnPtr);
        installed |= lifecycle::kCaptureLayers;
    }
    return installed;
}

}  // namespace

CaptureInstallReport InstallJniCaptureHooks(zygisk::Api* api,
                                            JNIEnv* env,
                                            int sdk) noexcept {
    CaptureInstallReport report{
            0,
            lifecycle::ProfileId::kNone,
            false,
    };
    if (api == nullptr || env == nullptr || env->ExceptionCheck()) {
        return report;
    }

    CaptureFields fields{};
    const CaptureProfile* profile = ProbeProfile(env, sdk, &fields);
    if (profile == nullptr) return report;

    g_fields = fields;
    report.profile = profile->id;
    report.hook_attempted = true;
    report.installed = HookProfile(api, env, *profile);
    return report;
}

}  // namespace zsc::capture
