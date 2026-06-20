#!/system/bin/sh

MODDIR=${0%/*}
STATE_DIR="$MODDIR/.boot_guard"
MARKER="$STATE_DIR/boot_in_progress"
COUNT_FILE="$STATE_DIR/failed_boot_count"
MAX_FAILURES=3

mkdir -p "$STATE_DIR"
chmod 0700 "$STATE_DIR"

count=0
if [ -f "$COUNT_FILE" ]; then
    count=$(cat "$COUNT_FILE" 2>/dev/null)
    case "$count" in
        ''|*[!0-9]*) count=0 ;;
    esac
fi

if [ -f "$MARKER" ]; then
    count=$((count + 1))
else
    count=0
fi

echo "$count" > "$COUNT_FILE"
: > "$MARKER"

if [ "$count" -ge "$MAX_FAILURES" ]; then
    touch "$MODDIR/disable"
    rm -f "$MARKER"
    log -t ZygiskSecureCapture "Auto-disabled after $count incomplete boots"
fi
