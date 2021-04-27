# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies data logging capability.
"""

from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port

# Initialize the motor.
motor = Motor(Port.A)

# Rotate the motor to base position.
motor.run_target(500, 0)
wait(500)

# Allocate logs for motor and controller signals.
DURATION = 2000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Rotate the motor.
motor.run_target(500, 360)

# Wait so we can also log hold capability, then turn off the motor completely.
wait(100)
motor.stop()

# Save the data by printing it. Pybricksdev will capture the printed
# data and save it to the given path relative to this script.
motor.log.save("build/servo_data.txt")
motor.control.log.save("build/control_data.txt")

# The log can also be read from the user script. For fun, let's print the
# position and the estimated speed in a rotated plot. We show only 1 in 3
# samples here and scale the speed and angle values for better visibility.
for i in range(len(motor.log) / 3):

    # Read the (i * 3)th row in the data log.
    values = motor.log.get(i * 3)
    speed = values[7]
    angle = values[2]

    # Turn it into a line of spaces with * for speed and + for angle.
    line = [ord(" ")] * 100
    line[speed // 6] = ord("*")
    line[angle // 5] = ord("+")

    # Print it. Looked at it sideways, you should see a trapezoidal graph
    # for speed and a gradual ramp for the angle.
    print(bytes(line))
