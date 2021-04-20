# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: 1

Description: Verify color sensor arguments,
    by asserting that the same color is reported by:
    - using the default argument, e.g. surface=True
    - and using NO argument.
"""

from pybricks.pupdevices import ColorSensor
from pybricks.parameters import Port

# Initialize device.
color_sensor = ColorSensor(Port.B)

# Verify that the default and NO-argument calls report the same color.
# Assume that the lighting will not change a lot during measuring.
hsv_default = color_sensor.hsv()
hsv_surface_true = color_sensor.hsv(surface=True)  # the default is surface=True.
# do not use surface=False in this test, that causes a mode switch and that would need more time.
assert hsv_default == hsv_surface_true, "Expected default '{0}' == '{1}'".format(
    hsv_default, hsv_surface_true
)

color_default = color_sensor.color()
color_surface_true = color_sensor.color(surface=True)  # the default is surface=True.
assert color_default == color_surface_true, "Expected default '{0}' == '{1}'".format(
    color_default, color_surface_true
)
