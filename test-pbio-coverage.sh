#!/bin/sh
#
# Shortcut for running coverage on the pbio library.
#

set -e

SCRIPT_DIR=$(dirname "$0")

export PBIO_TEST_RESULTS_DIR="results"
rm -rf "${SCRIPT_DIR}/lib/pbio/test/${PBIO_TEST_RESULTS_DIR}"/*
make -s -C "${SCRIPT_DIR}/lib/pbio/test" coverage-html

xdg-open "${SCRIPT_DIR}/lib/pbio/test/build-coverage/html/index.html"
