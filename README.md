# Zygisk Secure Capture

Experimental **Zygisk-only** module for allowing Android system screenshot paths to include secure layers without requiring LSPosed.

> **Development status:** v0.1 draft. This repository is not yet a tested release. Review the code and test on a recoverable device before flashing.

## v0.1 scope

- Hooks screenshot capture primitives in `system_server` only.
- Runtime-probes explicit AOSP JNI signatures instead of relying only on `SDK_INT`.
- Supports the known AOSP layouts used by:
  - Android 12–13: `android.view.SurfaceControl`.
  - Android 14: `android.window.ScreenCapture` with the two-argument layer native.
  - Android 15–16: `android.window.ScreenCapture` with the `sync` layer argument.
- Forces `mAllowProtected = false` before enabling `mCaptureSecureLayers = true`.
- Fails open when a class, field, or native signature is unavailable.
- Unloads from every app process; v0.1 remains loaded only in `system_server`.
- Includes a root-companion configuration channel and a three-boot auto-disable guard.

## Deliberately not implemented yet

- Sanitizing `ScreenshotHardwareBuffer.containsSecureLayers` metadata.
- Screenshot-observer suppression.
- Screen-recording callback suppression.
- Vendor-specific screenshot services.
- Binder `relayout` fallback.

Because metadata sanitization is not present in v0.1, some screenshot callers may still reject or refuse to persist a captured buffer even when the secure pixels were included.

## Safety boundary

This project does **not** enable protected or DRM-backed buffer capture. The hook writes `mAllowProtected = false` on every intercepted capture request before writing `mCaptureSecureLayers = true`.

Use only on a device and content you are authorized to control. Keep a working recovery path available.

## Build

Requirements:

- Android NDK with `ndk-build`.
- `curl` or `wget` to obtain the published Zygisk module API header.
- `zip` for the flashable package.

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk
./build.sh
```

Outputs are written to `out/`.

## Configuration

Edit `module/config/default.conf` before packaging, or edit the installed copy under:

```text
/data/adb/modules/zygisk_secure_capture/config/default.conf
```

Current option:

```ini
capture_secure_layers=true
```

## License

Project code is licensed under `GPL-3.0-only`. See `LICENSE` and `THIRD_PARTY_NOTICES.md`.
