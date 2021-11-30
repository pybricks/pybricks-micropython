#!/bin/sh
#
# Shortcut for running tests on the pbio library.
#
# Run `./test-pbio.sh --list-tests` for more info
#

set -e

SCRIPT_DIR=$(dirname "$0")

export PBIO_TEST_RESULTS_DIR="${SCRIPT_DIR}/lib/pbio/test/results"
rm -rf "${PBIO_TEST_RESULTS_DIR:?}"/*
make -s -C "${SCRIPT_DIR}/lib/pbio/test" -j$(nproc)

ulimit -S -c unlimited

set +e

"${SCRIPT_DIR}/lib/pbio/test/build/test-pbio" "$@"

# use gdb to get backtrace on segfault
if [ $? -eq 139 ]; then
    gdb -q "${SCRIPT_DIR}/lib/pbio/test/build/test-pbio" "${PBIO_TEST_RESULTS_DIR}/core" -ex backtrace
fi
