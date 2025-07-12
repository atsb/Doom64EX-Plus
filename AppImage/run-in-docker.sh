#!/bin/sh
#shellcheck disable=SC2086

set -xe

patch_appimage_elf_header() {
    # replace bytes 8-10 of the ELF header with zeros
    # https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html#elfid
    dd if=/dev/zero of=$1 bs=1 count=3 seek=8 conv=notrunc
}

make clean

# -fPIC and -pie are needed on aarch64 for linuxdeploy to work (otherwise ldd on DOOM64Ex-Plus crashes)
CFLAGS="$(rpm -E '%optflags') -fPIC -DDOOM_UNIX_INSTALL" LDFLAGS="-pie" $(rpm -E '%make_build')

ARCH=$(arch)
DEPLOYBIN=linuxdeploy-${ARCH}.AppImage
DEPLOYBIN_PLUGIN=linuxdeploy-plugin-appimage-${ARCH}.AppImage

cd AppImage

if [ ! -x $DEPLOYBIN ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20250213-2/$DEPLOYBIN \
         https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/1-alpha-20250213-1/$DEPLOYBIN_PLUGIN
    chmod +x $DEPLOYBIN $DEPLOYBIN_PLUGIN

    if [ "$ARCH" = "aarch64" ]; then
        # need to patch ELF header of linuxdeploy binaries for them to run on qemu on aarch64
        # otherwise it mysteriously fails with "cannot execute binary file: Exec format error"
        patch_appimage_elf_header $DEPLOYBIN
        patch_appimage_elf_header $DEPLOYBIN_PLUGIN
    fi
fi

ln -sf ../libfmod.so.??

APPDIR=AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp ../doom64ex-plus.wad "${APPDIR}/usr/bin"

./$DEPLOYBIN --appdir "$APPDIR" --executable ../DOOM64EX-Plus --desktop-file DOOM64EX-Plus.desktop  --icon-file DOOM64EX-Plus.png --output appimage

chown -R $HOST_UID:$HOST_GID .
