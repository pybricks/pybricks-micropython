#!/bin/bash
set -e
project="pybricks-ev3rt"

abs_path() {
    # https://stackoverflow.com/a/3915420/1976323
    echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")"
}

script_dir=$(dirname $(abs_path "${0}"))

if ! which docker >/dev/null; then
    echo "Error: Docker is not installed"
    exit 1
fi

src_dir="${script_dir}/../../../../.." # micropython directory
build_dir="${script_dir}/../build"
image_name="${project}"
container_name="${project}"

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
    "${script_dir}/"

docker rm --force ${container_name} >/dev/null 2>&1 || true
docker run \
    --volume "$(abs_path ${src_dir}):/src" \
    --workdir /src/ports/pybricks/bricks/ev3rt \
    --name ${container_name} \
    --env "TERM=${TERM}" \
    --user $(id -u):$(id -g) \
    --tty \
    --detach \
    ${image_name} tail

docker exec --tty ${container_name} bash -c '\
    cd ev3rt-hrp2/cfg && \
    make && \
    cd ../base-workspace && \
    make app=loader && \
    cd ../../ \
'

echo "Done. You can now compile by running 'docker exec --tty ${container_name} make'"
