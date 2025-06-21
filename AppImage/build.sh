#!/bin/sh

if [ -z "$FMOD_STUDIO_SDK_ROOT" ]; then
   echo "FMOD_STUDIO_SDK_ROOT environment variable is not set. Point it to the root of the FMOD studio SDK that can be downloaded on https://www.fmod.com/download#fmodstudio)"
   exit 1
fi

set -xe

IMAGE_NAME=doom64-build

docker build --progress=plain -t $IMAGE_NAME .

# if linuxdeploy complains about FUSE, add --priviledged

docker run \
       --rm -it \
       --device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined \
       -v ..:/Doom64EX-Plus \
       -v "$FMOD_STUDIO_SDK_ROOT":"$FMOD_STUDIO_SDK_ROOT" \
       -e FMOD_STUDIO_SDK_ROOT \
       -e HOST_UID=$(id -u) \
       -e HOST_GID=$(id -g) \
       $IMAGE_NAME
