#

# syntax=docker/dockerfile:1

FROM ubuntu:22.04

ENV LANG C.UTF-8

# software-properties-common provides apt-add-repository

RUN export DEBIAN_FRONTEND=noninteractive ; mkdir -m 777 /src && \
    apt-get update && \
    apt-get install -y curl git python3 python3-pip build-essential qemu-user-static pipx ruby u-boot-tools && \
    apt-get install -y software-properties-common && \
    apt-add-repository ppa:pybricks/ppa && \
    apt-get install -y uncrustify && \
    rm -rf /var/lib/apt/lists/*

# Install Poetry
RUN pipx install poetry

# For ev3rt
RUN gem install shell

# TODO
# https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz?rev=7bd049b7a3034e64885fa1a71c12f91d&hash=732D909FA8F68C0E1D0D17D08E057619

RUN arm_toolchain_version=$(curl https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads | sed -n 's!^.*<h4>Version \(.*\)</h4>.*$!\1!p') ; \
    curl -Lo gcc-arm-none-eabi.tar.xz https://developer.arm.com/-/media/Files/downloads/gnu/${arm_toolchain_version}/binrel/arm-gnu-toolchain-${arm_toolchain_version}-x86_64-arm-none-eabi.tar.xz && \
    mkdir -p /opt/gcc-arm-none-eabi && \
    tar xf gcc-arm-none-eabi.tar.xz --strip-components=1 -C /opt/gcc-arm-none-eabi && \
    echo 'export PATH=$PATH:/opt/gcc-arm-none-eabi/bin' | tee -a /etc/profile.d/gcc-arm-none-eabi.sh


ENV PATH=/opt/gcc-arm-none-eabi/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# RUN make $MAKEOPTS -C micropython/mpy-cross

WORKDIR /src 

ENTRYPOINT [ "/usr/bin/make" ]
