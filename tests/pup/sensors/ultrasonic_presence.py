# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1
"""

from pybricks.pupdevices import UltrasonicSensor
from pybricks.parameters import Port

# Initialize devices.
ultrasonic_sensor = UltrasonicSensor(Port.E)

# Assert that there are no ultrasonic devices nearby.
assert not ultrasonic_sensor.presence(), "Unexpected interference detected."
