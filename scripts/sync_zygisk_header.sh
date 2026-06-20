#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/module/jni/zygisk.hpp"
REVISION="${ZYGISK_HEADER_REV:-master}"
URL="https://raw.githubusercontent.com/topjohnwu/zygisk-module-sample/${REVISION}/module/jni/zygisk.hpp"
EXPECTED_SHA256="${ZYGISK_HEADER_SHA256:-}"

is_valid_header() {
  local file="$1"
  grep -q '^#define ZYGISK_API_VERSION 4$' "$file" &&
    grep -q 'REGISTER_ZYGISK_MODULE' "$file" &&
    grep -q 'REGISTER_ZYGISK_COMPANION' "$file"
}

if [[ -s "$DEST" ]] && is_valid_header "$DEST"; then
  exit 0
fi

mkdir -p "$(dirname "$DEST")"
TMP="$DEST.tmp"
rm -f "$TMP"
trap 'rm -f "$TMP"' EXIT HUP INT TERM

if command -v curl >/dev/null 2>&1; then
  curl -fL --retry 3 --connect-timeout 15 "$URL" -o "$TMP"
elif command -v wget >/dev/null 2>&1; then
  wget -O "$TMP" "$URL"
else
  echo "curl or wget is required to download zygisk.hpp" >&2
  exit 1
fi

if ! is_valid_header "$TMP"; then
  echo "Downloaded file is not the expected Zygisk API v4 header" >&2
  exit 1
fi

if [[ -n "$EXPECTED_SHA256" ]]; then
  ACTUAL_SHA256="$(sha256sum "$TMP" | awk '{print $1}')"
  if [[ "$ACTUAL_SHA256" != "$EXPECTED_SHA256" ]]; then
    echo "zygisk.hpp SHA-256 mismatch" >&2
    echo "expected: $EXPECTED_SHA256" >&2
    echo "actual:   $ACTUAL_SHA256" >&2
    exit 1
  fi
fi

mv "$TMP" "$DEST"
trap - EXIT HUP INT TERM
