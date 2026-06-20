#include "android11_systemui_hook.hpp"

#include "../common/attributes.hpp"
#include "../common/jni_utils.hpp"
#include "../common/log.hpp"
#include "../lifecycle/capability_registry.hpp"
#include "../zygisk.hpp"

namespace zsc::capture {
namespace {

constexpr char kSurfaceControlClass[] = "android/view/SurfaceControl";
constexpr char kNativeScreenshotName[] = "nativeScreenshot";
constexpr char kNativeScreenshotSignature[] =
        "(Landroid/os/IBinder;Landroid/graphics/Rect;IIZIZ)"
        "Landroid/view/SurfaceControl$ScreenshotGraphicBuffer;";

using NativeScreenshotFn = jobject (*)(JNIEnv*, jclass, jobject, jobject,
                                       jint, jint, jboolean, jint, jboolean);

void* g_original_native_screenshot = nullptr;
uint32_t g_missing_original_log_once = 0;

ZSC_ALWAYS_INLINE NativeScreenshotFn LoadOriginal() noexcept {
    return reinterpret_cast<NativeScreenshotFn>(
            __atomic_load_n(&g_original_native_screenshot,
                            __ATOMIC_ACQUIRE));
}

ZSC_HOT jobject HookNativeScreenshot(
        JNIEnv* env, jclass clazz, jobject display_token,
        jobject source_crop, jint width, jint height,
        jboolean use_identity_transform, jint rotation,
        jboolean capture_secure_layers) {
    (void)capture_secure_layers;
    NativeScreenshotFn original = LoadOriginal();
    if (ZSC_UNLIKELY(original == nullptr)) {
        ZSC_LOGE_ONCE(
                g_missing_original_log_once,
                "Android 11 screenshot hook observed a missing original pointer");
        return nullptr;
    }

    // Android 11 exposes secure-layer capture as a boolean argument. It does
    // not enable protected/DRM buffers; that is a separate graphics path.
    return original(env, clazz, display_token, source_crop, width, height,
                    use_identity_transform, rotation, JNI_TRUE);
}

bool Preflight(JNIEnv* env) noexcept {
    jclass clazz = common::FindClassNoThrow(env, kSurfaceControlClass);
    if (clazz == nullptr) return false;

    const jmethodID method = common::GetStaticMethodIdNoThrow(
            env, clazz, kNativeScreenshotName, kNativeScreenshotSignature);
    env->DeleteLocalRef(clazz);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return method != nullptr;
}

}  // namespace

uint32_t InstallAndroid11SystemUiCaptureHook(zygisk::Api* api,
                                             JNIEnv* env) noexcept {
    if (api == nullptr || env == nullptr || env->ExceptionCheck() ||
        !Preflight(env)) {
        return 0;
    }

    JNINativeMethod method{
            const_cast<char*>(kNativeScreenshotName),
            const_cast<char*>(kNativeScreenshotSignature),
            reinterpret_cast<void*>(HookNativeScreenshot),
    };
    api->hookJniNativeMethods(env, kSurfaceControlClass, &method, 1);
    if (method.fnPtr == nullptr) return 0;

    __atomic_store_n(&g_original_native_screenshot, method.fnPtr,
                     __ATOMIC_RELEASE);
    return lifecycle::kCaptureDisplay;
}

}  // namespace zsc::capture
