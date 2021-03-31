# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies that the Ultrasonic Sensor does not sense ultrasonic
interference by default.
"""
from pybricks.pupdevices import UltrasonicSensor
from pybricks.parameters import Port

# Initialize devices.
ultrasonic_sensor = UltrasonicSensor(Port.C)

# Assert that there are no ultrasonic devices nearby.
assert not ultrasonic_sensor.presence(), "Unexpected interference detected."
