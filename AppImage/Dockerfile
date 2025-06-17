FROM registry.opensuse.org/opensuse/leap:15.6
LABEL maintainer="bubbleguuum"

RUN zypper ar -f https://download.opensuse.org/repositories/games/15.6 games

RUN zypper --gpg-auto-import-keys \
    in -y \
    wget libtool make file fuse gcc 'pkgconfig(sdl3)' 'pkgconfig(libpng)'

WORKDIR /Doom64EX-Plus

CMD AppImage/run-in-docker.sh
