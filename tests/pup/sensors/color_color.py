# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1
"""

from pybricks.pupdevices import Motor, ColorSensor, UltrasonicSensor
from pybricks.parameters import Port, Color

# Initialize devices.
color_sensor = ColorSensor(Port.A)
motor = Motor(Port.C)
ultrasonic_sensor = UltrasonicSensor(Port.E)
SPEED = 500

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

# Verify saturated colors without calibration.
for name in ("GREEN", "BLUE", "RED", "YELLOW"):
    motor.run_target(SPEED, angles[name])
    detected = color_sensor.color()
    assert detected == Color[name], "Expected {0} but got {1}".format(Color[name], detected)

# Update all colors.
for name in angles.keys():
    motor.run_target(SPEED, angles[name])
    Color[name] = color_sensor.hsv()

# Set new colors as detectable colors.
color_sensor.detectable_colors([Color[key] for key in angles.keys()])

# Test all newly calibrated colors.
for name in angles.keys():
    motor.run_target(SPEED, angles[name])
    detected = color_sensor.color()
    assert detected == Color[name], "Expected {0} but got {1}".format(Color[name], detected)
