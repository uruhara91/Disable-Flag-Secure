# Feature Installer Contract

Every new backend—whether inspired by DisableFlagSecure, ih8SecureLock, AOSP, or vendor firmware—must
satisfy this contract before it is enabled by default.

## Scope

1. Declare the exact process role required.
2. Keep the library only when the feature is configured and its backend is compiled.
3. Never install a global app hook when a `system_server` or screenshot-service hook can solve the
   same problem.

## Probe before mutation

1. Resolve class, field, method, symbol, transaction code, and schema requirements first.
2. Do not use `SDK_INT` as the sole compatibility decision.
3. Do not leave a hook from a rejected candidate profile.
4. Treat unknown vendor changes as unsupported, not “close enough”.

## Hook safety

1. Save and validate every original pointer.
2. Publish shared pointers/state before the hook can be used whenever the backend supports an
   atomic commit.
3. If a backend cannot avoid a narrow publication race, its wrapper must return an error instead
   of crashing.
4. Once any hook API is invoked in a retained process, never unload the module library.

## Hot path

1. Start with the cheapest discriminator, usually a transaction code or exact method entry.
2. No file I/O, configuration parsing, unbounded allocation, or routine logging.
3. Delay descriptor/schema parsing until cheaper conditions match.
4. Precompute immutable replies or metadata when practical.

## Data validation

1. Every native buffer or Parcel view carries an explicit size.
2. Check overflow before addition, multiplication, alignment, or `len + 1` operations.
3. Validate negative lengths, truncation, object tables, interface descriptors, and reply shape.
4. On any mismatch, invoke original behavior without modification.

## Feature isolation

1. Return attempted/installed/failed capability bits.
2. Never make an unrelated feature fail because one adapter is unavailable.
3. Log installation state once; do not log each screenshot, relayout, transaction, or callback.

## Content boundary

`mAllowProtected` must remain false. No backend may enable Widevine/protected buffers, bypass
hardware-backed media protection, or reinterpret unreadable protected content as ordinary pixels.
