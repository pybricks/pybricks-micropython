FROM ev3dev/debian-stretch-cross
RUN sudo apt-get update && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get install --yes --no-install-recommends \
        libffi-dev:armel \
        pkg-config \
        python \
        python3
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabi/pkgconfig
