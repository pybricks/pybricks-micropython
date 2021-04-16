# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: 1

Description: Verifies motor trajectory generation.
"""

from pybricks.pupdevices import Motor
from pybricks.parameters import Port

# Initialize devices.
motor = Motor(Port.A)

# Basic trajectories starting from zero. It would be nice to compute them
# here in the loop so we can also include random tests, vary acceleration, etc.
trajectories = [
    ((500, 90), (0, 244, 244, 488, 0, 45, 45, 90, 0, 367, 1500, -1500)),
    ((1000, 720), (0, 666, 720, 1386, 0, 333, 387, 720, 0, 1000, 1500, -1500)),
    ((2000, 720), (0, 666, 720, 1386, 0, 333, 387, 720, 0, 1000, 1500, -1500)),
    ((10, 720), (0, 6, 72006, 72012, 0, 0, 720, 720, 0, 10, 1500, -1500)),
    ((-500, 360), (0, 333, 721, 1054, 0, -83, -277, -360, 0, -500, -1500, 1500)),
    ((500, -360), (0, 333, 721, 1054, 0, -83, -277, -360, 0, -500, -1500, 1500)),
    ((-500, -360), (0, 333, 721, 1054, 0, 83, 277, 360, 0, 500, 1500, -1500)),
    ((500, 1), (0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1500, -1500)),
    ((500, 0), (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)),
]

# Assert that all trajectories are correct.
for (speed, angle), trajectory in trajectories:
    # Start from 0.
    motor.reset_angle(0)

    # Initiate run_target command, stopping immediately.
    motor.run_angle(speed, angle, wait=False)
    result = motor.control.trajectory()
    motor.stop()

    # Compare generated trajectory to saved trajectory.
    assert trajectory == result, "Bad trajectory: {0} for {1}".format(result, (speed, angle))
