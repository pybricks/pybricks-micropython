# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies motor readings such as angle, absolute angle, and speed.
"""

from pybricks.pupdevices import Motor, UltrasonicSensor
from pybricks.parameters import Port, Direction
from pybricks.tools import wait, StopWatch

# Initialize devices.
motor = Motor(Port.A)
motor = Motor(port=Port.A, positive_direction=Direction.CLOCKWISE, gears=[])
ultrasonic_sensor = UltrasonicSensor(Port.C)

# Assert initial values.
assert motor.speed() == 0, "Unexpected initial motor speed."
assert -180 <= motor.angle() <= 179, "Unexpected initial motor angle."

# Verify driving to zero.
for target in (-360, 0, 360):
    motor.run_target(500, target)
    assert ultrasonic_sensor.distance() < 100, "Unexpected distance result."

# Test angle reset.
for reset_value in (-98304, -360, -180, -1.234, 0, 0.1, 1, 178, 180, 13245687):
    motor.reset_angle(reset_value)
    assert motor.angle() == int(reset_value), "Incorrect angle reset"

# Test motion after reset. Also test kwarg.
motor.reset_angle(angle=180)
for target in (-180, 180):
    motor.run_target(500, target)
    assert ultrasonic_sensor.distance() < 100, "Unexpected reset result."

# Test reset to absolute value, given that sensor sees object at the top.
motor.reset_angle()
assert -5 <= motor.angle() <= 5, "Unexpected absolute motor angle."

# Test absolute angle for sign change by putting it at +90 degrees clockwise.
motor.run_target(500, 90)
assert 85 <= motor.angle() <= 95, "Unable to put motor in +90 position."
motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
assert -85 >= motor.angle() >= -95, "Unexpected angle after CCW init."

# Test DC positive direction signs.
for direction in (Direction.CLOCKWISE, Direction.COUNTERCLOCKWISE, "default"):

    # Initialize the motor with given sign.
    if direction == "default":
        motor = Motor(Port.A)
    else:
        motor = Motor(Port.A, direction)

    old_angle = motor.angle()
    motor.dc(100)
    wait(1000)
    assert motor.angle() > old_angle + 90
    motor.dc(0)
    wait(500)

# The motor is now in positive orientation. Test DC forward.
old_angle = motor.angle()
motor.dc(50)
wait(1000)
assert motor.angle() > old_angle + 45
motor.dc(0)
wait(500)

# The motor is now in positive orientation. Test DC backward.
old_angle = motor.angle()
motor.dc(-50)
wait(1000)
assert motor.angle() < old_angle - 45
motor.dc(0)
wait(500)

# Test reported speed value.
watch = StopWatch()
for dc in (-100, 50, 0, 50, 100):

    # Get the motor going at steady speed.
    motor.dc(dc)
    wait(500)

    # Measure position delta.
    old_angle = motor.angle()

    # Get reported speed.
    watch.reset()
    reported_speed = []
    for i in range(100):
        reported_speed.append(motor.speed())
        wait(10)

    # Calculate real speed.
    real_speed = (motor.angle() - old_angle) / watch.time() * 1000

    # Compare with reported speed.
    for value in reported_speed:
        assert abs(value - real_speed) <= 100
