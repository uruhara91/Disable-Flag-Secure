# Architecture

## 1. Lifecycle invariant

The process-retention decision is made in `preAppSpecialize` or `preServerSpecialize`, before a
hook can be installed.

- Irrelevant process: request `DLCLOSE_MODULE_LIBRARY` immediately.
- Retained process: never request `DLCLOSE_MODULE_LIBRARY` later, including when probing or a
  partial installation fails.

This prevents dangling callbacks when a loader reports a partial hook failure or when one feature
installer succeeds before another fails.

## 2. Process roles

The policy layer knows four app-side roles:

- SystemUI.
- Vendor screenshot service.
- Explicit target application.
- Irrelevant process.

Exclusions always win. v0.1 compiles one app-side backend: the exact Android 11 AOSP SystemUI
screenshot adapter. On API 30, only the exact `com.android.systemui` process reaches companion IPC.
Every unrelated app process unloads immediately. On API 31–36, capture runs in `system_server` and
ordinary app processes unload immediately.

Future versions must retain an app or vendor process only when a corresponding post-app installer
is compiled and enabled. Reserved configuration flags alone must never keep a process resident.

## 3. Configuration snapshot

The root companion reads configuration once per companion process with `pthread_once`, then sends
a fixed-size immutable snapshot. The snapshot contains:

- Magic and protocol version.
- Structure sizes.
- Feature flags.
- Sorted, deduplicated 64-bit package/process hashes.
- A checksum over the complete structure.

The receiver validates every field and falls back to an all-disabled snapshot on transport or
validation failure. File I/O never occurs in a capture hot path. Features without compiled
backends are disabled by default in both the packaged configuration and the native fallback
snapshot.

## 4. Feature manager

Every feature installer returns:

- Attempted functional capability bits.
- Installed functional capability bits.
- Failed functional capability bits.
- One selected runtime profile ID.
- Whether any hook API was invoked.

Profile identity is not encoded as a functional capability bit. The manager can therefore compare
requested functionality without accidentally treating a profile marker as an installed feature.
One adapter may fail without disabling an unrelated adapter.

## 5. Capture profiles

Profile selection is non-destructive. Before the first hook call, the installer verifies the full
contract for exactly one candidate profile:

1. Capture argument class exists.
2. `mAllowProtected` exists and is boolean.
3. `mCaptureSecureLayers` exists and is boolean.
4. The exact display native signature exists.
5. Exactly one supported layer native signature exists.

Known AOSP profiles:

1. Android 11: `com.android.systemui` and
   `android/view/SurfaceControl.nativeScreenshot(..., captureSecureLayers)`.
2. Android 12–13: `android/view/SurfaceControl` with object listener arguments.
3. Android 14: `android/window/ScreenCapture`, layer signature ending `;J)I`.
4. Android 15–16: `android/window/ScreenCapture`, layer signature ending `;JZ)I`.

Android 15–16 is probed before Android 14 because both share the same class and display signature.
A candidate is rejected before mutation unless both exact native methods are present.

After a profile is selected, display and layer methods are hooked independently. The field IDs are
published before any hook can run. Each wrapper loads its original function pointer with acquire
semantics before mutating the request; the installer stores returned original pointers with release
semantics immediately after each hook call. A missing pointer returns an error without touching
`CaptureArgs`.

On API 31–36, every successful mutation writes `mAllowProtected=false` before
`mCaptureSecureLayers=true`. A failure of the protected-content write prevents the secure-layer
write. Android 11 uses its separate `captureSecureLayers` boolean and does not enable protected
buffers.

## 6. Planned system-first expansion

```text
system_server
├── AOSP secure capture engine             [v0.1]
├── capability and compatibility probes    [future]
└── independent feature reports

SystemUI / vendor screenshot service
├── secure metadata compatibility adapter  [future]
└── vendor capture adapter                 [future]

target applications only
├── callback compatibility diagnostics     [future]
└── bounded relayout fallback               [last resort]
```

The `relayout` path must never become the default. It requires transaction-code fast paths,
validated descriptors, bounded parsing, valid normal framework behavior on every mismatch, and
version/ROM-specific schemas.
