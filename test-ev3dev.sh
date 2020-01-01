#!/bin/sh
#
# Runs tests on ev3dev port.
#
# Use `--list-test` to list tests or `--include <regex>` to run single tests.
#

set -e

SCRIPT_DIR=$(dirname "$0")
BRICK_DIR="$SCRIPT_DIR/bricks/ev3dev"
MP_TEST_DIR="$SCRIPT_DIR/../../tests"
PB_TEST_DIR=$(readlink -f "$SCRIPT_DIR/tests")

make -s -j $(nproc --all) -C "$BRICK_DIR" CROSS_COMPILE=

export MICROPY_MICROPYTHON="$PB_TEST_DIR/ev3dev/test-wrapper.sh"

cd "$MP_TEST_DIR"
./run-tests --test-dirs $(find "$PB_TEST_DIR/ev3dev" -type d) "$@" || "$PB_TEST_DIR/dump-out-files.sh"
