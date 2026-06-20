# Zygisk Secure Capture

Experimental **Zygisk-only** foundation for allowing Android screenshot paths to include
secure layers without requiring LSPosed. The architecture is system-first: prefer a narrowly
scoped capture hook in `system_server`, then add screenshot-service adapters and target-app
fallbacks only when a platform path cannot cover the requirement.

> **Status:** v0.1 development foundation. It has not yet been built on CI or validated on a
> physical device. Keep a working recovery path and do not treat this branch as a release.

## Non-negotiable safety boundary

The module never enables protected/DRM-backed buffer capture. Every intercepted request writes:

```text
mAllowProtected = false
mCaptureSecureLayers = true
```

The protected-content write happens first. Unknown classes, signatures, fields, process roles,
configuration snapshots, and future Binder layouts fail open to original Android behavior.

## v0.1 scope

- Zygisk public API v4 only; no loader-specific internals.
- `system_server` JNI hooks for AOSP capture primitives.
- Runtime profile probing for:
  - Android 12–13: `android.view.SurfaceControl`.
  - Android 14: `android.window.ScreenCapture` two-argument layer native.
  - Android 15–16: `android.window.ScreenCapture` layer native with `sync`.
- Exact method preflight before any hook is installed.
- Independent display/layer capability reporting.
- Immutable, checksummed root-companion configuration snapshot.
- Bounded and validated `targets.txt` / `exclude.txt` parsing for later versions.
- Automatic unload from every app process because v0.1 has no app-side backend.
- Three-incomplete-boot automatic disable guard.
- Hardened multi-ABI build and artifact verification.

## Deliberately deferred

- `ScreenshotHardwareBuffer.containsSecureLayers` metadata sanitization.
- Android 14+ screenshot-observer suppression.
- Android 15+ screen-recording callback suppression.
- One UI, HyperOS, OPlus, and other vendor adapters.
- Binder `IWindowSession.relayout` fallback.

The deferred components already have process roles, feature flags, capability bits, and feature
contracts reserved for them. They should be added as isolated installers rather than folded into
one global hook.

## Why the foundation differs from the SystemUI module

`SystemUI-MediaMetadata-NPE-Fix` is intentionally fingerprint-locked, arm64-only, and optimized
around one Android 12 Binder layout. This project retains its useful engineering rules—early
scope decisions, exact preflight, resident-library safety, minimal release logs, and ELF/ZIP
verification—but replaces firmware constants with runtime capability profiles and independent
feature backends.

## Build

Linux/macOS with Android NDK:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
./build.sh
```

Windows PowerShell:

```powershell
.\build.ps1 -NdkPath 'C:\Android\Sdk\ndk\27.2.12479018'
```

The flashable development ZIP is written to `dist/`.

For a reproducible reviewed release, pin the Zygisk header:

```bash
export ZYGISK_HEADER_REV='<immutable commit>'
export ZYGISK_HEADER_SHA256='<reviewed sha256>'
./build.sh
```

## Configuration

Installed configuration lives under:

```text
/data/adb/modules/zygisk_secure_capture/config/
```

`default.conf` contains all planned feature switches. In v0.1 only
`capture_secure_layers` has an implementation. App-side switches remain inert until their
backends are compiled.

## Recovery

From a root shell or recovery:

```sh
touch /data/adb/modules/zygisk_secure_capture/disable
reboot
```

The boot guard performs the same disable action after three consecutive boots that never reach
`sys.boot_completed=1`.

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- [`docs/FEATURE_CONTRACT.md`](docs/FEATURE_CONTRACT.md)
- [`docs/TESTING.md`](docs/TESTING.md)

## License

GPL-3.0-only. See `LICENSE` and `THIRD_PARTY_NOTICES.md`.
