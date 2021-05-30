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


# The logs can also be read from the user script. As a check, let's print the
# distance and the estimated speed in a rotated plot. We show only 1 in 5
# samples here and scale the speed and angle values for better visibility.

start_distance = drive_base.distance_control.log.get(0)[2]

for i in range(len(drive_base.distance_control.log) / 5):

    # Read the (i * 3)th row in the data log.
    values = drive_base.distance_control.log.get(i * 5)
    speed = values[9]
    angle = values[2] - start_distance

    # Turn it into a line of spaces with * for speed and + for angle.
    line = [ord(" ")] * 100
    line[min(speed // 10, 99)] = ord("*")
    line[min(angle // 36, 99)] = ord("+")

    # Print it. Looked at it sideways, you should see a trapezoidal graph
    # for speed and a gradual ramp for the angle as it drives forward.
    print(bytes(line))
