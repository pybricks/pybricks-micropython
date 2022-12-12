#!/bin/bash
#
# Runs tests on virtualhub.
#
# Use `--list-test` to list tests or `--include <regex>` to run single tests.
#

set -e

if [[ $CI != "true" ]]; then
    NOT_CI="true"
fi

SCRIPT_DIR=$(readlink -f "$(dirname "$0")")
BRICK_DIR="$SCRIPT_DIR/bricks/virtualhub"
MP_TEST_DIR="$SCRIPT_DIR/micropython/tests"
PB_TEST_DIR=$"$SCRIPT_DIR/tests"
BUILD_DIR="$BRICK_DIR/build${COVERAGE:+-coverage}"
PBIO_DIR="$SCRIPT_DIR/lib/pbio"

make -s -j $(nproc --all) -C "$BRICK_DIR"

export MICROPY_MICROPYTHON="$BUILD_DIR/virtualhub-micropython"
export PYTHONPATH="$PBIO_DIR/cpython"
export PBIO_VIRTUAL_PLATFORM_MODULE=pbio_virtual.platform.robot

cd "$MP_TEST_DIR"
./run-tests.py --test-dirs $(find "$PB_TEST_DIR/virtualhub" -type d) "$@" || \
    (code=$?; ./run-tests.py --print-failures; exit $code)

if [[ $COVERAGE ]]; then
    lcov --capture --output-file "$BUILD_DIR/lcov.info" \
            --directory "$BUILD_DIR" \
            --exclude "**/micropython/**" \
            --exclude "/usr/**" \
            ${NOT_CI:+--quiet}

    if [[ -z $CI ]]; then
        # if not on CI, generate html for local viewing
        genhtml "$BUILD_DIR/lcov.info" --output-directory "$BUILD_DIR/html" --quiet
    fi
fi
