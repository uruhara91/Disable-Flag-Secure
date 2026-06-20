#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
NDK_BUILD=${NDK_BUILD:-${ANDROID_NDK_HOME:-}/ndk-build}

if [ ! -x "$NDK_BUILD" ]; then
    echo "ndk-build not found. Set ANDROID_NDK_HOME or NDK_BUILD." >&2
    exit 1
fi

"$ROOT_DIR/scripts/sync_zygisk_header.sh"

OUT_DIR="$ROOT_DIR/out"
OBJ_DIR="$OUT_DIR/obj"
LIB_DIR="$OUT_DIR/libs"
PKG_DIR="$OUT_DIR/package"

rm -rf "$OBJ_DIR" "$LIB_DIR" "$PKG_DIR"
mkdir -p "$OBJ_DIR" "$LIB_DIR" "$PKG_DIR/zygisk"

"$NDK_BUILD" \
    NDK_PROJECT_PATH="$ROOT_DIR/module" \
    APP_BUILD_SCRIPT="$ROOT_DIR/module/jni/Android.mk" \
    NDK_APPLICATION_MK="$ROOT_DIR/module/jni/Application.mk" \
    NDK_OUT="$OBJ_DIR" \
    NDK_LIBS_OUT="$LIB_DIR"

for abi in arm64-v8a armeabi-v7a x86_64; do
    src="$LIB_DIR/$abi/libzygisk_secure_capture.so"
    if [ -f "$src" ]; then
        cp "$src" "$PKG_DIR/zygisk/$abi.so"
    fi
done

cp "$ROOT_DIR/module/module.prop" "$PKG_DIR/"
cp "$ROOT_DIR/module/customize.sh" "$PKG_DIR/"
cp "$ROOT_DIR/module/post-fs-data.sh" "$PKG_DIR/"
cp "$ROOT_DIR/module/service.sh" "$PKG_DIR/"
cp -R "$ROOT_DIR/module/config" "$PKG_DIR/"

chmod 0755 "$PKG_DIR/customize.sh" "$PKG_DIR/post-fs-data.sh" "$PKG_DIR/service.sh"

if command -v zip >/dev/null 2>&1; then
    (
        cd "$PKG_DIR"
        zip -qr "$OUT_DIR/zygisk-secure-capture-v0.1-dev.zip" .
    )
    echo "Package: $OUT_DIR/zygisk-secure-capture-v0.1-dev.zip"
else
    echo "zip not found; unpacked package is at $PKG_DIR" >&2
fi
