#!/bin/sh

set -e

project="pybricks-ev3dev"

abs_path() {
    # https://stackoverflow.com/a/3915420/1976323
    echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")"
}

script_dir=$(dirname $(abs_path "${0}"))

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

src_dir="${script_dir}/../../../../.."
build_dir="${script_dir}/../build-${arch}"
image_name="${project}-${arch}"
container_name="${project}_${arch}"

if [ -e ${build_dir} ]; then
    read -r -n 1 -p "Delete existing build directory at '$(abs_path ${build_dir})'? (y/n/q) "
    echo
    case $REPLY in
        y|Y)
            rm -rf "${build_dir}"
            ;;
        q|Q)
            exit 0
            ;;
        *)
            ;;
    esac
fi
mkdir -p ${build_dir}

docker build \
    --tag ${image_name} \
    --no-cache \
    --file "${script_dir}/${arch}.dockerfile" \
    "${script_dir}/"

docker rm --force ${container_name} >/dev/null 2>&1 || true
docker run \
    --volume "$(abs_path ${src_dir}):/src" \
    --workdir /src/ports/pybricks/bricks/ev3dev \
    --name ${container_name} \
    --env "TERM=${TERM}" \
    --env "DESTDIR=/build/dist" \
    --user $(id -u):$(id -g) \
    --tty \
    --detach \
    ${image_name} tail

echo "Done. You can now compile by running 'docker exec --tty ${container_name} make'"
