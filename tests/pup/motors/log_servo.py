# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies data logging capability.
"""

from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port

# Initialize the motor.
motor = Motor(Port.A)

# Rotate the motor to base position.
motor.run_target(500, 0)
wait(500)

# Allocate logs for motor and controller signals.
DURATION = 2000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Rotate the motor.
motor.run_target(500, 360)

# Wait so we can also log hold capability, then turn off the motor completely.
wait(1000)
motor.stop()

# Save the data by printing it. Pybricksdev will capture the printed
# data and save it to the given path relative to this script.
motor.log.save('build/servo_data.txt')
motor.control.log.save('build/control_data.txt')
