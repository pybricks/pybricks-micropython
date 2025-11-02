#!/bin/bash
#
# Runs tests on simhub.
#
# Use `--list-test` to list tests or `--include <regex>` to run single tests.
#

set -e

if [[ $CI != "true" ]]; then
    NOT_CI="true"
fi

SCRIPT_DIR=$(readlink -f "$(dirname "$0")")
BRICK_DIR="$SCRIPT_DIR/bricks/simhub"
MP_TEST_DIR="$SCRIPT_DIR/micropython/tests"
PB_TEST_DIR=$"$SCRIPT_DIR/tests"
BUILD_DIR="$BRICK_DIR/build${COVERAGE:+-coverage}"
PBIO_DIR="$SCRIPT_DIR/lib/pbio"

make -s -j $(nproc --all) -C "$BRICK_DIR" COPT=-DPBDRV_CONFIG_RUN_ON_CI

export MICROPY_MICROPYTHON="$BUILD_DIR/firmware.elf"

cd "$MP_TEST_DIR"
./run-tests.py --test-dirs $(find "$PB_TEST_DIR/virtualhub" -type d -and ! -wholename "*/build/*"  -and ! -wholename "*/run_test.py") "$@" || \
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
