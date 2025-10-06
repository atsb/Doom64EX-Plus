FROM registry.opensuse.org/opensuse/leap:15.5
LABEL maintainer="bubbleguuum"

RUN set -eux; \
    packages="wget libtool make file gcc pkgconfig(libpng) SDL3-devel"; \
    zypper ar -gcf https://download.opensuse.org/repositories/home:/bobbie424242/15.5 SDL3; \
    zypper --gpg-auto-import-keys in -y $packages

WORKDIR /Doom64EX-Plus

CMD AppImage/run-in-docker.sh
