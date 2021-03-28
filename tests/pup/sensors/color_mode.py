# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

UART sensors can yield stale data after a mode change. This test reproduces
this scenario and verifies that built-in time delay works around it.
"""

from pybricks.pupdevices import Motor, ColorSensor, UltrasonicSensor
from pybricks.parameters import Port, Color
from pybricks.tools import wait

# Initialize devices.
color_sensor = ColorSensor(Port.A)
motor = Motor(Port.C)
ultrasonic_sensor = UltrasonicSensor(Port.E)
SPEED = 500

# Initialize sensor to make sure we're in the expected mode
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

# Go to green and assert that we really do see green
motor.run_target(SPEED, angles["BLUE"])
assert color_sensor.color() == Color["BLUE"]

# Read ambient light
color_sensor.ambient()
wait(3000)

# Go to red and read the color
motor.run_target(SPEED, angles["RED"])
detected = color_sensor.color()
assert detected == Color["RED"], "Expected {0} but got {1}".format(Color["RED"], detected)
