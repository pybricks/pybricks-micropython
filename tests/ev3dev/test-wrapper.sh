#!/bin/sh
#
# Runs pybricks-micropython with ev3dev-mocks-run for testing

DIR=$(dirname "$(readlink -f $0)")

export EV3DEV_MOCKS_UMOCKDEV_RUN_ARGS="-d $DIR/lego-ev3-large-motor-port-a.umockdev"

exec ev3dev-mocks-run "$DIR/../../bricks/ev3dev/pybricks-micropython" "$@"
