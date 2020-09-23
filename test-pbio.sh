#!/bin/sh
#
# Shortcut for running tests on the pbio library.
#
# Run ``./test-pbio.sh --list-tests` for more info
#

set -e

SCRIPT_DIR=$(dirname "$0")

export PBIO_TEST_RESULTS_DIR="${SCRIPT_DIR}/lib/pbio/test/results"
rm -rf "${PBIO_TEST_RESULTS_DIR:?}"/*
make -s -C "${SCRIPT_DIR}/lib/pbio/test"
"${SCRIPT_DIR}/lib/pbio/test/build/test-pbio" "$@"
