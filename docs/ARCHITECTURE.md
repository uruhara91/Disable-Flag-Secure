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

Exclusions always win. v0.1 compiles no app-side backend, so it bypasses configuration IPC and
unloads from every app process. Future versions must set `kAppFeatureBackendsCompiled=true` only
when at least one post-app installer exists.

## 3. Configuration snapshot

The root companion reads configuration once per companion process with `pthread_once`, then sends
a fixed-size immutable snapshot. The snapshot contains:

- Magic and protocol version.
- Structure sizes.
- Feature flags.
- Sorted, deduplicated 64-bit package/process hashes.
- A checksum over the complete structure.

The receiver validates every field and falls back to an all-disabled snapshot on transport or
validation failure. File I/O never occurs in a capture or Binder hot path.

## 4. Feature manager

Every feature installer returns:

- Attempted capability bits.
- Installed capability bits.
- Failed capability bits.
- Selected runtime profile.
- Whether any hook API was invoked.

The manager merges reports without making one feature depend on another. A vendor metadata adapter
must therefore be able to fail while the AOSP capture engine remains active.

## 5. Capture profiles

Profile selection is non-destructive. JNI classes and exact native signatures are resolved with
`GetStaticMethodID` before `hookJniNativeMethods` is called.

Known AOSP profiles:

1. Android 12–13: `android/view/SurfaceControl`.
2. Android 14: `android/window/ScreenCapture`, layer signature ending `;J)I`.
3. Android 15–16: `android/window/ScreenCapture`, layer signature ending `;JZ)I`.

Once selected, display and layer methods are hooked independently and their original pointers are
published with acquire/release atomics. A missing pointer returns an error rather than dereferencing
null.

## 6. Planned system-first expansion

```text
system_server
├── AOSP secure capture engine             [v0.1]
├── server-side detection filter           [future]
└── capability report

SystemUI / vendor screenshot service
├── secure metadata sanitizer              [future]
└── vendor capture adapter                 [future]

target applications only
├── screenshot observer shield             [future]
├── recording callback shield              [future]
└── bounded relayout fallback               [last resort]
```

The `relayout` path must never become the default. It requires transaction-code fast paths,
validated descriptors, bounded parsing, valid synthetic replies, and version/ROM-specific schemas.
