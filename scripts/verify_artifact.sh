#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <package-directory> <zip-file>" >&2
  exit 2
fi

PACKAGE_DIR="$1"
ZIP_FILE="$2"
NDK_ROOT="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"

if [[ -z "$NDK_ROOT" ]]; then
  echo "Set ANDROID_NDK_HOME or ANDROID_NDK_ROOT." >&2
  exit 1
fi

READELF="$(find "$NDK_ROOT/toolchains/llvm/prebuilt" -type f \( -name llvm-readelf -o -name llvm-readelf.exe \) | head -n 1)"
if [[ -z "$READELF" || ! -x "$READELF" ]]; then
  echo "llvm-readelf was not found in the Android NDK." >&2
  exit 1
fi

verify_elf() {
  local abi="$1"
  local machine="$2"
  local file="$PACKAGE_DIR/zygisk/$abi.so"

  [[ -f "$file" ]] || { echo "Missing $file" >&2; return 1; }
  "$READELF" -h "$file" | grep -q "$machine" || {
    echo "Unexpected ELF machine for $abi" >&2
    return 1
  }
  "$READELF" -Ws "$file" | grep -q 'zygisk_module_entry' || {
    echo "Missing zygisk_module_entry in $abi" >&2
    return 1
  }
  "$READELF" -Ws "$file" | grep -q 'zygisk_companion_entry' || {
    echo "Missing zygisk_companion_entry in $abi" >&2
    return 1
  }
  if "$READELF" -d "$file" | grep -q 'libc++_shared'; then
    echo "Unexpected libc++_shared dependency in $abi" >&2
    return 1
  fi
  "$READELF" -lW "$file" | grep -q 'GNU_RELRO' || {
    echo "Missing GNU_RELRO in $abi" >&2
    return 1
  }
  "$READELF" -d "$file" | grep -Eq 'BIND_NOW|FLAGS.*NOW' || {
    echo "Missing immediate binding in $abi" >&2
    return 1
  }
  if "$READELF" -lW "$file" | grep 'GNU_STACK' | grep -q 'RWE'; then
    echo "Executable stack detected in $abi" >&2
    return 1
  fi
}

verify_elf arm64-v8a AArch64
verify_elf armeabi-v7a ARM
verify_elf x86_64 'Advanced Micro Devices X86-64\|X86-64'

for required in module.prop customize.sh post-fs-data.sh service.sh config/default.conf; do
  [[ -f "$PACKAGE_DIR/$required" ]] || {
    echo "Missing package file: $required" >&2
    exit 1
  }
done

[[ -f "$ZIP_FILE" ]] || { echo "Missing ZIP: $ZIP_FILE" >&2; exit 1; }
unzip -Z1 "$ZIP_FILE" | grep -qx 'zygisk/arm64-v8a.so'
unzip -Z1 "$ZIP_FILE" | grep -qx 'zygisk/armeabi-v7a.so'
unzip -Z1 "$ZIP_FILE" | grep -qx 'zygisk/x86_64.so'
unzip -Z1 "$ZIP_FILE" | grep -qx 'module.prop'

echo "Artifact verification passed."
