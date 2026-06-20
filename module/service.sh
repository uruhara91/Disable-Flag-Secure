#!/system/bin/sh

MODDIR=${0%/*}
STATE_DIR="$MODDIR/.boot_guard"
MARKER="$STATE_DIR/boot_in_progress"
COUNT_FILE="$STATE_DIR/failed_boot_count"

# Wait up to 15 minutes. If Android never completes boot, leave the marker in
# place so the next boot counts as a failure.
i=0
while [ "$i" -lt 180 ]; do
    if [ "$(getprop sys.boot_completed)" = "1" ]; then
        rm -f "$MARKER"
        echo 0 > "$COUNT_FILE"
        exit 0
    fi
    i=$((i + 1))
    sleep 5
done

exit 0
