# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: 1

Description: Verify that ambient() and reflection() refuse an argument.
"""

from pybricks.pupdevices import ColorSensor
from pybricks.parameters import Port

# Initialize device.
color_sensor = ColorSensor(Port.B)

# Get the ambient light intensity to verify that the default works.
ambient_light_default = color_sensor.ambient()

# verify an argument passed to ambient is correctly refused.
expected = "function doesn't take keyword arguments"
try:
    ambient_light_surface_true = color_sensor.ambient(surface=True)
except Exception as e:
    assert str(e) == expected, "Expected '{0}' == '{1}'".format(e, expected)

# Get the reflected light intensity to verify that the default works.
reflection_default = color_sensor.reflection()

# verify an argument passed to reflection is correctly refused.
expected = "function doesn't take keyword arguments"
try:
    reflection_surface_true = color_sensor.reflection(surface=True)
except Exception as e:
    assert str(e) == expected, "Expected '{0}' == '{1}'".format(e, expected)
