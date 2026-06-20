#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NDK_ROOT="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
MODULE_PROP="$ROOT/module/module.prop"
VERSION="$(sed -n 's/^version=//p' "$MODULE_PROP" | head -n 1)"

if [[ -z "$NDK_ROOT" ]]; then
  echo "Set ANDROID_NDK_HOME or ANDROID_NDK_ROOT." >&2
  exit 1
fi
if [[ -z "$VERSION" ]]; then
  echo "Could not read version from $MODULE_PROP" >&2
  exit 1
fi

NDK_BUILD="$NDK_ROOT/ndk-build"
[[ -x "$NDK_BUILD" ]] || {
  echo "ndk-build was not found under $NDK_ROOT" >&2
  exit 1
}

"$ROOT/scripts/sync_zygisk_header.sh"
"$ROOT/scripts/run_host_tests.sh"

OUT="$ROOT/out"
OBJ="$OUT/obj"
LIBS="$OUT/libs"
PACKAGE="$OUT/package"
DIST="$ROOT/dist"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"

rm -rf "$OBJ" "$LIBS" "$PACKAGE"
mkdir -p "$OBJ" "$LIBS" "$PACKAGE/zygisk" "$DIST"

"$NDK_BUILD" \
  NDK_PROJECT_PATH="$ROOT/module" \
  APP_BUILD_SCRIPT="$ROOT/module/jni/Android.mk" \
  NDK_APPLICATION_MK="$ROOT/module/jni/Application.mk" \
  NDK_OUT="$OBJ" \
  NDK_LIBS_OUT="$LIBS" \
  -j"$JOBS"

for abi in arm64-v8a armeabi-v7a x86_64; do
  cp "$LIBS/$abi/libzygisk_secure_capture.so" "$PACKAGE/zygisk/$abi.so"
done

cp "$ROOT/module/module.prop" "$PACKAGE/"
cp "$ROOT/module/customize.sh" "$PACKAGE/"
cp "$ROOT/module/post-fs-data.sh" "$PACKAGE/"
cp "$ROOT/module/service.sh" "$PACKAGE/"
cp -R "$ROOT/module/config" "$PACKAGE/"
cp "$ROOT/LICENSE" "$PACKAGE/"

chmod 0755 "$PACKAGE/customize.sh" "$PACKAGE/post-fs-data.sh" "$PACKAGE/service.sh"

ZIP="$DIST/Zygisk-Secure-Capture-${VERSION}.zip"
rm -f "$ZIP"
(
  cd "$PACKAGE"
  zip -r9 "$ZIP" .
)

"$ROOT/scripts/verify_artifact.sh" "$PACKAGE" "$ZIP"
echo "Built module: $ZIP"
