# Architecture

## v0.1 execution model

1. Zygisk loads the library into a forked process.
2. Every ordinary app process immediately requests `DLCLOSE_MODULE_LIBRARY`.
3. `system_server` obtains a small immutable configuration record from the root companion during `preServerSpecialize`.
4. During `postServerSpecialize`, the module probes exact JNI classes, fields, and signatures.
5. A hook is retained only when Zygisk returns a non-null original function pointer.
6. Each intercepted request writes `mAllowProtected = false` first, then writes `mCaptureSecureLayers = true`, and invokes the original native function.

## Profiles

### Android 12–13 AOSP

Class: `android/view/SurfaceControl`

Capture argument class: `android/view/SurfaceControl$CaptureArgs`

### Android 14 AOSP

Class: `android/window/ScreenCapture`

Layer method signature ends in `;J)I`.

### Android 15–16 AOSP

Class: `android/window/ScreenCapture`

Layer method signature ends in `;JZ)I`, where the final boolean is the synchronous-capture flag.

## Failure policy

- Missing class: profile unavailable.
- Missing field: profile unavailable.
- Missing native signature: that capability remains disabled.
- JNI exception during argument mutation: clear the module-generated exception and call the original function.
- Unknown vendor implementation: do not patch it.

No Binder parser, ART method hook, or vendor adapter is present in v0.1.
