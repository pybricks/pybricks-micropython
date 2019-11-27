FROM ev3dev/debian-stretch-cross
RUN sudo apt-get update && \
    DEBIAN_FRONTEND=noninteractive sudo apt-get install --yes --no-install-recommends \
        build-essential \
        ev3dev-mocks \
        libasound2:armel \
        libasound2-plugin-ev3dev:armel \
        libffi-dev:armel \
        libmagickwand-6.q16-3:armel \
        libsndfile1:armel \
        libudev-dev:armel \
        libumockdev0:armel \
        pkg-config \
        python \
        python3 \
        libi2c-dev \
        uthash-dev:armel
RUN apt-get download umockdev:armel && \
    ar x umockdev*.deb data.tar.xz && \
    sudo tar -C / -xf data.tar.xz ./usr/lib/arm-linux-gnueabi/libumockdev-preload.so.0.0.0 && \
    sudo tar -C / -xf data.tar.xz ./usr/lib/arm-linux-gnueabi/libumockdev-preload.so.0 && \
    rm data.tar.xz && rm *.deb
# Hack to get correct linux/i2c-dev.h header file in cross compiler
# Kernel version of i2c-dev.h is in /usr/arm-linux-gnueabi/include/ which is first in search path
# thanks https://stackoverflow.com/a/36287466/1976323
RUN sudo mv /usr/arm-linux-gnueabi/include/linux/i2c-dev.h /usr/arm-linux-gnueabi/include/linux/i2c-dev.h.kernel && \
    sudo cp /usr/include/linux/i2c-dev.h /usr/arm-linux-gnueabi/include/linux/i2c-dev.h
ENV PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabi/pkgconfig
ENV CROSS_COMPILE=arm-linux-gnueabi-
ENV BUILD=build-armel
