# Testing and Release Gates

## Automated gates

`build.sh` performs:

1. Zygisk API v4 header validation.
2. Host hash test.
3. Multi-ABI NDK build.
4. Packaging.
5. ELF checks for architecture, exported Zygisk entries, no shared libc++ dependency, RELRO,
   immediate binding, and non-executable stack.
6. ZIP content validation.

GitHub Actions repeats the build using Android NDK 27.2.12479018.

## Device test order

1. Boot with all feature flags false.
2. Enable only `capture_secure_layers`.
3. Confirm normal screenshots still work.
4. Test one non-sensitive `FLAG_SECURE` sample application owned by the tester.
5. Repeat SystemUI restarts and at least three full reboots.
6. Collect only startup/install errors; hot-path logging remains disabled.
7. Add metadata sanitization only after pixel capture is stable.
8. Add observer/callback shields only after explicit target allowlisting works.
9. Add a vendor adapter on one firmware profile at a time.
10. Add Binder relayout fallback last and fuzz its bounded parser before device use.

## Failure expectations

- Unknown profile: no hook, original Android behavior.
- Partial JNI availability: installed methods continue; missing methods are reported failed.
- Invalid configuration snapshot: all features disabled.
- Three incomplete boots: module automatically disabled.
- Metadata sanitizer absent: secure pixels may be captured but a caller may still refuse to save.
