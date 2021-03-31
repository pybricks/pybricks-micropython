# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies reflection sensing, by asserting that the colors None,
Black, Red, and Yellow have ascending reflection values.
"""

from pybricks.pupdevices import Motor, ColorSensor
from pybricks.parameters import Port

# Initialize devices.
motor = Motor(Port.A)
color_sensor = ColorSensor(Port.B)
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

previous_reflection = 0

# Verify that these colors have ascending reflection values.
for name in ("NONE", "BLACK", "RED", "YELLOW"):
    motor.run_target(SPEED, angles[name])
    reflection = color_sensor.reflection()
    assert reflection > previous_reflection, "Expected {0} > {1}".format(
        reflection, previous_reflection
    )
