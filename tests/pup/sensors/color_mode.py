# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies that sensors do not give stale data after a mode change.
"""

from pybricks.pupdevices import Motor, ColorSensor
from pybricks.parameters import Port, Color
from pybricks.tools import wait

# Initialize devices.
motor = Motor(Port.A)
color_sensor = ColorSensor(Port.B)
SPEED = 500

# Initialize sensor to make sure we're in the expected mode.
color_sensor.color()
wait(500)

# Color angle targets
angles = {
    "GREEN": 20,
    "BLUE": 110,
    "RED": 200,
    "YELLOW": 290,
    "BLACK": 250,
    "WHITE": 75,
    "NONE": 162,
}

# Go to blue and assert that we really do see blue.
motor.run_target(SPEED, angles["BLUE"])
assert color_sensor.color() == Color["BLUE"]

# Read ambient light, causing a mode change.
color_sensor.ambient()
wait(3000)

# Go to red and read the color right away.
motor.run_target(SPEED, angles["RED"])
detected = color_sensor.color()

# With the built-in delay, the stale blue value should be gone, now giving red.
assert detected == Color["RED"], "Expected {0} but got {1}".format(Color["RED"], detected)
