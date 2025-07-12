#!/bin/sh
#shellcheck disable=SC2086,SC2046

if [ -z "$FMOD_STUDIO_SDK_ROOT" ]; then
   echo "FMOD_STUDIO_SDK_ROOT environment variable is not set. Point it to the root of the FMOD studio SDK that can be downloaded on https://www.fmod.com/download#fmodstudio)"
   exit 1
fi

set -xe

TARGET_ARCH=$1

[ -z "$TARGET_ARCH" ] && TARGET_ARCH=x86_64

case $TARGET_ARCH in
    x86_64)
        PLATFORM_ARCH=amd64
        ;;
    aarch64)
        PLATFORM_ARCH=arm64
        ;;
    *)
        echo "Unmanaged target arch: $TARGET_ARCH"
esac

IMAGE_NAME=doom64-build-$PLATFORM_ARCH

docker buildx build --progress=plain --platform=linux/$PLATFORM_ARCH --build-arg TARGET_ARCH=$TARGET_ARCH -t $IMAGE_NAME --load .

# if linuxdeploy complains about FUSE, add --privileged

docker run \
       --platform=linux/$PLATFORM_ARCH \
       --rm -it \
       --device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined \
       -v ..:/Doom64EX-Plus \
       -v "$FMOD_STUDIO_SDK_ROOT":"$FMOD_STUDIO_SDK_ROOT" \
       -e FMOD_STUDIO_SDK_ROOT \
       -e HOST_UID=$(id -u) \
       -e HOST_GID=$(id -g) \
       $IMAGE_NAME
