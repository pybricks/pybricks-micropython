FROM ubuntu:xenial
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
        software-properties-common && \
    add-apt-repository ppa:team-gcc-arm-embedded/ppa --yes --update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
        build-essential \
        gcc-arm-embedded \
        libboost1.58-all-dev \
        pkg-config \
        u-boot-tools
