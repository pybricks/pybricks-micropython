#!/bin/sh
#
# Run this script when tests fail to see if there is any useful output.

SEP="------------------------------------------------------------------------"
TEST_DIR=$(readlink -f $(dirname $(readlink -f $0))/../micropython/tests)

find ${TEST_DIR} -name "*.out" -exec echo {} \; -exec echo ${SEP} \; \
    -exec cat {} \; -exec echo \; -exec echo ${SEP} \; -exec echo \;
