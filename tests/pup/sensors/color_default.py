# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies color sensor arguments,
    by asserting that the same colors are reported by
		using the default argument, e.g. surface=True
		and using NO argument
"""

from pybricks.pupdevices import ColorSensor
from pybricks.parameters import Port

# Initialize devices.
color_sensor = ColorSensor(Port.B)

# verify that the default and NO-argument calls report the same color
# assume that the lighting will not change a lot during measuring
hsvD = color_sensor.hsv()
hsvT = color_sensor.hsv(surface=True)  # the default
# do not use surface=False in this test as that causes a mode switch
# and that would need more time
assert hsvD == hsvT, "Expected default '{0}' == '{1}'".format(hsvD, hsvT)

colorD = color_sensor.color()
colorT = color_sensor.color(surface=True)  # the default
assert colorD == colorT, "Expected default '{0}' == '{1}'".format(colorD, colorT)
