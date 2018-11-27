#!/bin/sh

set -e

project="pybricks-ev3"

script_dir=$(dirname $(readlink -f "${0}"))

case ${1} in
    armel)
        arch=${1}
        ;;
    *)
        echo "Error: Must specify 'armel'"
        exit 1
        ;;
esac

if ! which docker >/dev/null; then
    echo "Error: Docker is not installed"
    exit 1
fi

build_dir=build-${arch}
image_name="${project}-${arch}"
container_name="${project}_${arch}"

mkdir -p ${build_dir}

docker build \
    --tag ${image_name} \
    --no-cache \
    --file "${script_dir}/${arch}.dockerfile" \
    "${script_dir}/"

docker rm --force ${container_name} >/dev/null 2>&1 || true
