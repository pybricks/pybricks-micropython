#!/bin/sh
#
# Runs pybricks-micropython with ev3dev-mocks-run for testing

DIR="$(dirname $(readlink -f $0))"

exec ev3dev-mocks-run "$DIR/../../bricks/ev3dev/pybricks-micropython" "$@"
