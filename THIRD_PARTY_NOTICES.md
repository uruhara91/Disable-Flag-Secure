# Third-Party Notices

## Zygisk public module API header

The build helper obtains the unmodified public Zygisk API v4 header from
`topjohnwu/zygisk-module-sample`. The header carries its own permissive license notice.
For reproducible release builds, set `ZYGISK_HEADER_REV` to an immutable commit and
`ZYGISK_HEADER_SHA256` to the reviewed file digest.

## Android Open Source Project

The capture profile names, JNI signatures, and field names are independently implemented
from public Android framework and AOSP source licensed under Apache License 2.0.

## SystemUI-MediaMetadata-NPE-Fix

The engineering discipline of `uruhara91/SystemUI-MediaMetadata-NPE-Fix` informed the
foundation: decide process scope before hooking, keep the target library mapped after
installation begins, preflight exact runtime contracts, minimize hot-path work, and verify
release artifacts. Firmware-specific transaction codes, Parcel offsets, and process locks
from that project are not copied into this generic module.

## ih8SecureLock

`j-hc/ih8SecureLock` is used as a native/Zygisk behavioral reference. No source file from
that project is included in this v0.1 foundation.

## LSPosed/DisableFlagSecure

`LSPosed/DisableFlagSecure` is used as a feature-coverage and compatibility checklist. No
source file from that project is included in this v0.1 foundation. If direct GPLv3 code is
adapted later, attribution and corresponding-source obligations must be retained.
