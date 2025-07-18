#!/bin/sh
#shellcheck disable=SC2086,SC2046

IMAGE_NAME=bubblesoftapps/doom64ex-plus-build

if [ "$1" = "clean" ]; then
    rm -rf AppDir *.AppImage libfmod.so.??
    exit 0
fi

if [ -z "$FMOD_STUDIO_SDK_ROOT" ]; then
   echo "FMOD_STUDIO_SDK_ROOT environment variable is not set. Point it to the root of the FMOD studio SDK that can be downloaded on https://www.fmod.com/download#fmodstudio)"
   exit 1
fi

set -xe

# supported platforms: linux/amd64 and linux/arm64
# can be passed as first parameter
PLATFORM=${1:-linux/amd64}

docker run \
       --platform=$PLATFORM \
       --rm -it \
       -v ..:/Doom64EX-Plus \
       -v "$FMOD_STUDIO_SDK_ROOT":"$FMOD_STUDIO_SDK_ROOT" \
       -e FMOD_STUDIO_SDK_ROOT \
       -e HOST_UID=$(id -u) \
       -e HOST_GID=$(id -g) \
       $IMAGE_NAME
