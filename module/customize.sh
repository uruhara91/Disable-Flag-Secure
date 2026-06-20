#!/system/bin/sh

ui_print "- Installing Zygisk Secure Capture v0.1 development build"
ui_print "- This build is experimental; keep a recovery path available"

if [ "$ARCH" != "arm64" ] && [ "$ARCH" != "arm" ] && [ "$ARCH" != "x64" ]; then
    abort "Unsupported architecture: $ARCH"
fi

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/customize.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
