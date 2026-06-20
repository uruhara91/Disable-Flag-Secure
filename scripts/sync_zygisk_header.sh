#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT_DIR/module/jni/zygisk.hpp"
URL="https://raw.githubusercontent.com/topjohnwu/zygisk-module-sample/master/module/jni/zygisk.hpp"

if [ -s "$DEST" ]; then
    exit 0
fi

mkdir -p "$(dirname -- "$DEST")"
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

if ! grep -q "REGISTER_ZYGISK_MODULE" "$TMP"; then
    echo "Downloaded file does not look like the Zygisk public API header" >&2
    exit 1
fi

mv "$TMP" "$DEST"
trap - EXIT HUP INT TERM
