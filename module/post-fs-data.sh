#!/system/bin/sh

MODDIR=${0%/*}
STATE_DIR="$MODDIR/.boot_guard"
MARKER="$STATE_DIR/boot_in_progress"
COUNT_FILE="$STATE_DIR/failed_boot_count"
REASON_FILE="$STATE_DIR/disable_reason"
MAX_FAILURES=3

[ -f "$MODDIR/disable" ] && exit 0

umask 077
mkdir -p "$STATE_DIR" || exit 0
chmod 0700 "$STATE_DIR" 2>/dev/null

count=0
if [ -f "$COUNT_FILE" ]; then
  count="$(cat "$COUNT_FILE" 2>/dev/null)"
  case "$count" in
    ''|*[!0-9]*) count=0 ;;
  esac
fi

if [ -f "$MARKER" ]; then
  count=$((count + 1))
else
  count=0
fi

printf '%s\n' "$count" > "$COUNT_FILE"
printf '%s\n' "$(date +%s 2>/dev/null)" > "$MARKER"

if [ "$count" -ge "$MAX_FAILURES" ]; then
  printf '%s\n' "auto-disabled after $count incomplete boots" > "$REASON_FILE"
  touch "$MODDIR/disable"
  rm -f "$MARKER"
  log -t ZygiskSecureCapture "Auto-disabled after $count incomplete boots"
fi
