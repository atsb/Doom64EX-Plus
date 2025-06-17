#!/bin/sh
#shellcheck disable=SC2086

set -xe

make clean
CFLAGS="$(rpm -E '%optflags') -DDOOM_UNIX_INSTALL" $(rpm -E '%make_build')

ARCH=$(arch)
DEPLOYBIN=linuxdeploy-${ARCH}.AppImage
DEPLOYBIN_PLUGIN=linuxdeploy-plugin-appimage-${ARCH}.AppImage

cd AppImage

if [ ! -x $DEPLOYBIN ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20250213-2/$DEPLOYBIN \
         https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/1-alpha-20250213-1/$DEPLOYBIN_PLUGIN
    chmod +x $DEPLOYBIN $DEPLOYBIN_PLUGIN 
fi

ln -sf ../libfmod.so.??

APPDIR=AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp ../doom64ex-plus.wad "${APPDIR}/usr/bin"

./$DEPLOYBIN --appdir "$APPDIR" --executable ../DOOM64EX-Plus --desktop-file DOOM64EX-Plus.desktop  --icon-file DOOM64EX-Plus.png --output appimage

chown -R $HOST_UID:$HOST_GID .
