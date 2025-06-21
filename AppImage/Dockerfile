FROM registry.opensuse.org/opensuse/leap:15.5
LABEL maintainer="bubbleguuum"
COPY libSDL3-0-3.2.16-0.x86_64.rpm /tmp
COPY SDL3-devel-3.2.16-0.x86_64.rpm /tmp

RUN zypper --gpg-auto-import-keys \
    in -y \
    --allow-unsigned-rpm \
    wget libtool make file fuse gcc 'pkgconfig(libpng)' \
    /tmp/*.rpm

WORKDIR /Doom64EX-Plus

CMD AppImage/run-in-docker.sh
