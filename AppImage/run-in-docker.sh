#!/bin/sh
#shellcheck disable=SC2086

set -xe

GAME_BIN=DOOM64EX-Plus

patch_appimage_elf_header() {
    # replace bytes 8-10 of the ELF header with zeros
    # https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html#elfid
    dd if=/dev/zero of=$1 bs=1 count=3 seek=8 conv=notrunc
}

# if there is a newer version of SDL3 from OBS (Open Build Service), install it
zypper ref -r SDL3
zypper --no-refresh in -y SDL3-devel

make clean

# -fPIC and -pie are needed on aarch64 for linuxdeploy to work (otherwise ldd on DOOM64EX-Plus crashes)
CFLAGS="$(rpm -E '%optflags') -fPIC -DDOOM_UNIX_INSTALL" LDFLAGS="-pie" $(rpm -E '%make_build')

# remove debug info
strip -s $GAME_BIN

ARCH=$(uname -m)

DEPLOYBIN=linuxdeploy-${ARCH}.AppImage
DEPLOYBIN_PLUGIN=linuxdeploy-plugin-appimage-${ARCH}.AppImage

cd AppImage

trap 'chown -R $HOST_UID:$HOST_GID ..' EXIT

if [ ! -x $DEPLOYBIN ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20250213-2/$DEPLOYBIN \
         https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/1-alpha-20250213-1/$DEPLOYBIN_PLUGIN
    chmod +x $DEPLOYBIN $DEPLOYBIN_PLUGIN

    if [ "$ARCH" = "aarch64" ]; then
        # need to patch ELF header of linuxdeploy binaries for them to run on qemu on aarch64
        # otherwise it mysteriously fails with "cannot execute binary file: Exec format error"
        # https://github.com/AppImage/AppImageKit/issues/828
        patch_appimage_elf_header $DEPLOYBIN
        patch_appimage_elf_header $DEPLOYBIN_PLUGIN
    fi
fi

ln -sf ../libfmod.so.??

APPDIR=AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp ../doom64ex-plus.wad "${APPDIR}/usr/bin"

# for running linuxdeploy AppImage without needing FUSE
export APPIMAGE_EXTRACT_AND_RUN=1

./$DEPLOYBIN --appdir "$APPDIR" --executable ../$GAME_BIN --desktop-file DOOM64EX-Plus.desktop  --icon-file DOOM64EX-Plus.png --output appimage

# normalize AppImage filename, including version
VERSION=$(grep "FileVersion" ../src/engine/doom64ex-plus.rc | cut -d ' ' -f 3 | tr -d '"')
mv $GAME_BIN-$ARCH.AppImage doom64ex-plus-$VERSION-linux-$ARCH.AppImage 


