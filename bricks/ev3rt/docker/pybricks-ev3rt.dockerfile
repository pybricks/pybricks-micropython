FROM ubuntu:xenial
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends --allow-unauthenticated \
        software-properties-common && \
    add-apt-repository ppa:team-gcc-arm-embedded/ppa -y && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends --allow-unauthenticated \
        gcc-arm-embedded \
        u-boot-tools \
        build-essential \
        pkg-config \        
        libboost1.58-all-dev 
ENV PKG_CONFIG_PATH=/usr/lib/pkgconfig
