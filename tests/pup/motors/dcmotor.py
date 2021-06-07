# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: Any hub with a DC motor on port D.

Description: Verifies DCMotor class.
"""

from pybricks.pupdevices import DCMotor
from pybricks.parameters import Port, Direction
from pybricks.tools import wait

# Initialize the motor.
motor = DCMotor(Port.D)

# Speed up, slow down, reverse.
for i in tuple(range(0, 150)) + tuple(range(150, -150, -1)):
    motor.dc(i)
    wait(30)

# Brake from full speed.
motor.brake()
wait(1000)

# Coast from full speed.
motor = DCMotor(Port.D, positive_direction=Direction.COUNTERCLOCKWISE)
motor.dc(100)
wait(1000)
motor.stop()
