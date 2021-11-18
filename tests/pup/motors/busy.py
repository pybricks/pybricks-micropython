# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Checks that EBUSY is raised when changing settings while holding.
"""

from pybricks.pupdevices import Motor
from pybricks.parameters import Port
from uerrno import EBUSY

# Initialize the motor.
motor = Motor(Port.A)

# Rotate the motor to base position and hold it there.
motor.run_target(500, 0)

# Try to change a setting. This should raise an error because the motor is busy.
try:
    motor.control.limits(torque=500)
except Exception as e:
    exception = e
    pass

# Assert that we got the expected exception.
assert exception.args[0] == EBUSY, "Did not raise expected exception."
