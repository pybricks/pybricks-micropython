# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1
"""

from pybricks.pupdevices import Motor, ColorSensor, UltrasonicSensor
from pybricks.parameters import Port

# Initialize devices.
color_sensor = ColorSensor(Port.A)
motor = Motor(Port.C)
ultrasonic_sensor = UltrasonicSensor(Port.E)

# Detect object.
motor.run_target(500, 0)
assert ultrasonic_sensor.distance() < 100, "Expected low distance."

# Move object away.
motor.run_target(500, 180)
assert ultrasonic_sensor.distance() > 100, "Expected high distance."
