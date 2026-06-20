#!/system/bin/sh

MODDIR=${0%/*}
STATE_DIR="$MODDIR/.boot_guard"
MARKER="$STATE_DIR/boot_in_progress"
COUNT_FILE="$STATE_DIR/failed_boot_count"
LAST_SUCCESS="$STATE_DIR/last_successful_boot"

[ -f "$MODDIR/disable" ] && exit 0

# Leave the marker intact when Android never completes boot. The next boot then
# increments the failure count in post-fs-data.sh.
i=0
while [ "$i" -lt 240 ]; do
  if [ "$(getprop sys.boot_completed)" = "1" ]; then
    rm -f "$MARKER"
    printf '0\n' > "$COUNT_FILE"
    printf '%s\n' "$(date +%s 2>/dev/null)" > "$LAST_SUCCESS"
    exit 0
  fi
  i=$((i + 1))
  sleep 5
done

exit 0
