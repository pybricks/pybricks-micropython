# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: 
	Verify that ambient() and reflection() refuse an argument
"""

from pybricks.pupdevices import ColorSensor
from pybricks.parameters import Port

# Initialize devices.
color_sensor = ColorSensor(Port.B)

# Get the ambient light intensity.
ambientD = color_sensor.ambient()

# verify an argument passed to ambient is refused
expected = "function doesn't take keyword arguments"
try:
    ambientT = color_sensor.ambient(surface=True)
except Exception as e:
    assert str(e) == expected, "Expected '{0}' == '{1}'".format(e, expected)

# Get the reflected light intensity.
reflectionD = color_sensor.reflection()

# verify an argument passed to ambient is refused
expected = "function doesn't take keyword arguments"
try:
    reflectionT = color_sensor.reflection(surface=True)
except Exception as e:
    assert str(e) == expected, "Expected '{0}' == '{1}'".format(e, expected)
