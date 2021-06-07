# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: Any medium-sized drive base with two angular medium motors.

Description: Verifies data logging capability for a drive base.
"""

from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase

# Initialize the motor and drive base.
left_motor = Motor(Port.C, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.D)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=44, axle_track=14 * 8)

# Allocate logs for motors and controller signals.
DURATION = 6000
left_motor.log.start(DURATION)
right_motor.log.start(DURATION)
drive_base.distance_control.log.start(DURATION)
drive_base.heading_control.log.start(DURATION)

# Drive for half a meter.
drive_base.straight(500)

# Wait so we can also log hold capability, then turn off the motor completely.
wait(100)
drive_base.stop()

# Save the data by printing it. Pybricksdev will capture the printed
# data and save it to the given path relative to this script.
left_motor.log.save("build/log_drive_base_left_motor.txt")
right_motor.log.save("build/log_drive_base_right_motor.txt")
drive_base.distance_control.log.save("build/log_drive_base_distance_control.txt")
drive_base.heading_control.log.save("build/log_drive_base_heading_control.txt")
