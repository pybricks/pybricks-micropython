#!/bin/sh
#
# Runs tests on ev3dev port.
#
# Use `--list-test` to list tests or `--include <regex>` to run single tests.
#

set -e

SCRIPT_DIR=$(dirname "$0")
BRICK_DIR="$SCRIPT_DIR/bricks/ev3dev"
MP_TEST_DIR="$SCRIPT_DIR/micropython/tests"
PB_TEST_DIR=$(readlink -f "$SCRIPT_DIR/tests")
BUILD_DIR=$(readlink -f "$BRICK_DIR/build-test")

rm -f "$BRICK_DIR/pybricks-micropython"
make -s -j $(nproc --all) -C "$BRICK_DIR" pybricks-micropython build-test/libgrx-3.0-vdriver-test.so CROSS_COMPILE= DEBUG=1 BUILD=build-test

export MICROPY_MICROPYTHON="$PB_TEST_DIR/ev3dev/test-wrapper.sh"
export GRX_PLUGIN_PATH="$BUILD_DIR"
export GRX_DRIVER=test

cd "$MP_TEST_DIR"
./run-tests.py --test-dirs $(find "$PB_TEST_DIR/ev3dev" -type d) "$@" || ./run-tests.py --print-failures
