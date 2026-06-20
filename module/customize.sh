#!/system/bin/sh

MIN_SDK=30
MAX_SDK=36
CURRENT_SDK="$(getprop ro.build.version.sdk)"
CURRENT_ABIS="$(getprop ro.product.cpu.abilist)"

ui_print "********************************"
ui_print " Zygisk Secure Capture v0.1-dev "
ui_print "********************************"
ui_print "* SDK: $CURRENT_SDK"
ui_print "* ABI list: $CURRENT_ABIS"
ui_print "* Experimental system-first foundation"

case "$CURRENT_SDK" in
  ''|*[!0-9]*) abort "! Could not determine Android SDK" ;;
esac
[ "$CURRENT_SDK" -ge "$MIN_SDK" ] && [ "$CURRENT_SDK" -le "$MAX_SDK" ] || \
  abort "! Supported SDK range is $MIN_SDK-$MAX_SDK"

found_abi=false
printf '%s' "$CURRENT_ABIS" | grep -q 'arm64-v8a' && \
  [ -f "$MODPATH/zygisk/arm64-v8a.so" ] && found_abi=true
printf '%s' "$CURRENT_ABIS" | grep -q 'armeabi-v7a' && \
  [ -f "$MODPATH/zygisk/armeabi-v7a.so" ] && found_abi=true
printf '%s' "$CURRENT_ABIS" | grep -q 'x86_64' && \
  [ -f "$MODPATH/zygisk/x86_64.so" ] && found_abi=true
[ "$found_abi" = true ] || abort "! No compatible Zygisk library was packaged"

if [ "$APATCH" = "true" ] || [ "$KERNELPATCH" = "true" ]; then
  ui_print "* APatch/FolkPatch-compatible manager detected"
  ui_print "* Zygisk Next or ReZygisk is required"
elif [ -n "$KSU" ]; then
  ui_print "* KernelSU-compatible manager detected"
  ui_print "* Zygisk Next or ReZygisk is required"
else
  ui_print "* Magisk-compatible manager detected"
  ui_print "* Enable a compatible Zygisk implementation"
fi

if [ "$CURRENT_SDK" -eq 30 ]; then
  ui_print "* Android 11 path: exact SystemUI nativeScreenshot adapter"
else
  ui_print "* Android 12-16 path: system_server capture adapter"
fi
ui_print "* Protected and DRM-backed capture remains disabled"
ui_print "* Automatic disable follows three incomplete boots"
ui_print "* Keep a recovery path available"
ui_print "* Reboot is required"

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/customize.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
