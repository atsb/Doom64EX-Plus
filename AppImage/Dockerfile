FROM registry.opensuse.org/opensuse/leap:15.5
LABEL maintainer="bubbleguuum"

ARG TARGET_ARCH

COPY libSDL3-0-3.2.16-0.${TARGET_ARCH}.rpm /tmp
COPY SDL3-devel-3.2.16-0.${TARGET_ARCH}.rpm /tmp

RUN set -eux; \
    packages="wget libtool make file fuse gcc bsdiff pkgconfig(libpng)"; \
    zypper --gpg-auto-import-keys in -y --allow-unsigned-rpm $packages /tmp/*.rpm

WORKDIR /Doom64EX-Plus

CMD AppImage/run-in-docker.sh
