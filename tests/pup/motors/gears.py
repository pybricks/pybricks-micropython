# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies gears argument for motors.
"""

from pybricks.pupdevices import Motor
from pybricks.parameters import Port, Stop
from pybricks.tools import wait


def get_gear_ratio(gear_arg):
    """Converts gear lists to gear ratio."""
    # Default ratio:
    if gear_arg is None:
        return 1.0

    # Ratio for single gear train.
    if type(gear_arg[0]) not in (list, tuple):
        return gear_arg[-1] / gear_arg[0]

    # Ratio for multiple gear trains.
    ratio = 1.0
    for train in gear_arg:
        ratio *= train[-1] / train[0]
    return ratio


# Gear arguments to test.
test_args = [
    None,
    [1, 3],
    (1, 3),
    [1, 2, 3],
    [1, 28],
    [12, 36],
    [36, 12],
    [[1, 24], [12, 36]],
    [(1, 24), (12, 36)],
    [(1, 24), [12, 36]],
    [[57, 24], [12, 123456789, 1]],
]

# Initialize device to known position.
motor = Motor(Port.A, gears=None)
motor.run_target(500, 170)
wait(500)
motor.dc(0)
real_angle = motor.angle()

# Test expected result.
for gears in test_args:
    # Initialize motors with new settings.
    motor = Motor(Port.A, gears=gears)

    # Get expected and measured value.
    expected = real_angle / get_gear_ratio(gears)
    measured = motor.angle()

    # Allow at most one degree of error. We could be a bit more precise
    # here and also test the expected rounding direction, by investigating
    # how this is currently defined. For now, 1 degree both ways will do.
    assert abs(motor.angle() - expected) <= 1, "{0} != {1}".format(expected, measured)
