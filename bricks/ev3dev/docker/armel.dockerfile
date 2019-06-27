FROM ev3dev/debian-stretch-cross
RUN sudo apt-get update && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get install --yes --no-install-recommends \
        build-essential \
        ev3dev-mocks:armel \
        libasound2:armel \
        libffi-dev:armel \
        libmagickwand-6.q16-3:armel \
        libsndfile1:armel \
        libudev-dev:armel \
        pkg-config \
        python \
        python3
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabi/pkgconfig
ENV CROSS_COMPILE=arm-linux-gnueabi-
ENV BUILD=build-armel
