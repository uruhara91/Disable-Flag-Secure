#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <package-directory> <zip-file>" >&2
  exit 2
fi

PACKAGE_DIR="$1"
ZIP_FILE="$2"
NDK_ROOT="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
MAX_LIBRARY_BYTES=$((512 * 1024))
EXPECTED_EXPORTS=$'zygisk_companion_entry\nzygisk_module_entry'
EXPECTED_NEEDED=$'libc.so\nliblog.so'

if [[ -z "$NDK_ROOT" ]]; then
  echo "Set ANDROID_NDK_HOME or ANDROID_NDK_ROOT." >&2
  exit 1
fi

READELF=""
while IFS= read -r candidate; do
  if [[ -x "$candidate" ]]; then
    READELF="$candidate"
    break
  fi
done < <(find "$NDK_ROOT/toolchains/llvm/prebuilt" \
  \( -type f -o -type l \) \
  \( -name llvm-readelf -o -name llvm-readelf.exe \) \
  -print 2>/dev/null)

if [[ -z "$READELF" ]]; then
  echo "llvm-readelf was not found in the Android NDK." >&2
  exit 1
fi

verify_elf() {
  local abi="$1"
  local machine="$2"
  local file="$PACKAGE_DIR/zygisk/$abi.so"
  local size
  local exports
  local needed

  [[ -f "$file" ]] || { echo "Missing $file" >&2; return 1; }

  size="$(wc -c < "$file")"
  if [[ "$size" -gt "$MAX_LIBRARY_BYTES" ]]; then
    echo "Unexpectedly large module for $abi: $size bytes" >&2
    return 1
  fi

  "$READELF" -h "$file" | grep -q "$machine" || {
    echo "Unexpected ELF machine for $abi" >&2
    return 1
  }

  exports="$(
    "$READELF" -Ws "$file" |
      awk '$5 == "GLOBAL" && $7 != "UND" {print $8}' |
      sort -u
  )"
  if [[ "$exports" != "$EXPECTED_EXPORTS" ]]; then
    echo "Unexpected exported symbols in $abi:" >&2
    printf '%s\n' "$exports" >&2
    return 1
  fi

  needed="$(
    "$READELF" -d "$file" |
      sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' |
      sort -u
  )"
  if [[ "$needed" != "$EXPECTED_NEEDED" ]]; then
    echo "Unexpected shared-library dependencies in $abi:" >&2
    printf '%s\n' "$needed" >&2
    return 1
  fi

  if "$READELF" -d "$file" | grep -Eq 'TEXTREL|RPATH|RUNPATH'; then
    echo "Forbidden dynamic tag detected in $abi" >&2
    return 1
  fi
  if "$READELF" -S "$file" | grep -Eq '\.debug_|\.symtab'; then
    echo "Debug or full symbol table detected in $abi" >&2
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
  if "$READELF" -lW "$file" | grep '^  LOAD' | grep -q 'RWE'; then
    echo "Writable executable LOAD segment detected in $abi" >&2
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
ZIP_ENTRIES="$(unzip -Z1 "$ZIP_FILE")"
printf '%s\n' "$ZIP_ENTRIES" | grep -qx 'zygisk/arm64-v8a.so'
printf '%s\n' "$ZIP_ENTRIES" | grep -qx 'zygisk/armeabi-v7a.so'
printf '%s\n' "$ZIP_ENTRIES" | grep -qx 'zygisk/x86_64.so'
printf '%s\n' "$ZIP_ENTRIES" | grep -qx 'module.prop'

if printf '%s\n' "$ZIP_ENTRIES" | grep -Eq '(^/|(^|/)\.\.(/|$)|^__MACOSX/|\.tmp$)'; then
  echo "Unsafe or temporary path detected in module ZIP" >&2
  exit 1
fi
if [[ -n "$(printf '%s\n' "$ZIP_ENTRIES" | sort | uniq -d)" ]]; then
  echo "Duplicate path detected in module ZIP" >&2
  exit 1
fi

echo "Artifact verification passed."
