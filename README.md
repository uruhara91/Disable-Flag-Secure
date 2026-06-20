# Zygisk Secure Capture

Experimental Zygisk-only secure-layer capture module. The implementation is being developed as a small, fail-open, system-first alternative that does not require LSPosed.

> **Status:** repository bootstrap. Do not flash a release until the v0.1 implementation has been built and reviewed.

The module never enables protected/DRM buffer capture. `mAllowProtected` must remain `false`.
