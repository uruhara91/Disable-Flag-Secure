#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CXX="${CXX:-c++}"
OUT="${TMPDIR:-/tmp}/zsc-host-tests-$$"
trap 'rm -f "$OUT"' EXIT HUP INT TERM

"$CXX" -std=c++20 -O2 -Wall -Wextra -Wpedantic \
  -I"$ROOT/module/jni" "$ROOT/tests/hash_test.cpp" -o "$OUT"
"$OUT"
