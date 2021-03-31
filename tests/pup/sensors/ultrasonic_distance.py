# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies the distance values of the Ultrasonic Sensor.
It rotates the motor to place an obstacle in front of the sensor to test
distance values. Then it rotates quickly to verify faster readings.
"""

from pybricks.pupdevices import Motor, UltrasonicSensor
from pybricks.parameters import Port


# Initialize devices.
motor = Motor(Port.A)
ultrasonic_sensor = UltrasonicSensor(Port.C)

# Detect object.
motor.run_target(500, 0)
distance = ultrasonic_sensor.distance()
assert distance < 100, "Expected < 100 mm, got {0}.".format(distance)

# Move object away.
motor.run_target(500, 180)
distance = ultrasonic_sensor.distance()
assert distance > 100, "Expected > 100 mm, got {0}.".format(distance)

# Prepare fast detection.
motor.reset_angle(0)
motor.run(700)
DETECTIONS = 5

# Wait for given number of detections.
for i in range(DETECTIONS):
    # Wait for object to be detected.
    while ultrasonic_sensor.distance() > 100:
        pass

    # Wait for object to move away.
    angle_detected = motor.angle()
    while motor.angle() < angle_detected + 180:
        pass


# Assert that we have made as many turns.
rotations = round(motor.angle() / 360)
assert rotations == DETECTIONS, "Expected {0} turns, got {1}.".format(DETECTIONS, rotations)
