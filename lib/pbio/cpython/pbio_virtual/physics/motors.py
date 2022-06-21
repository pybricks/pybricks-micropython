# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from numpy import array
from math import degrees

from .simulation import SimulationModel


class SimpleMotor(SimulationModel):
    """Simplest representation of a DC motor with idealized angular sensors."""

    # Angle alpha (rad) and its time derivative (rad/s)
    STATES = ("alpha", "alpha_dot")

    # The input is the duty cycle (-1.0 to 1.0)
    INPUTS = ("duty",)

    # The output is the angle (mdeg).
    OUTPUTS = ("angle",)

    # Out-of-bounds numeric "duty" value used to indicate coast.
    COAST_DUTY = 10

    # System constants
    c0 = 22.48
    c1 = 0.0455 * 10000

    def output(self, t, x):
        # Only position is measured as an output.
        alpha, alpha_dot = x
        return array([degrees(alpha) * 1000])

    def state_change(self, t, x, u):
        # Unpack the current state.
        alpha, alpha_dot = x
        (duty,) = u

        # Evaluate equation of motion, which is just the acceleration.
        if duty == self.COAST_DUTY:
            # For coast, reduce deceleration, as if removing back EMF.
            alpha_dotdot = -self.c0 * alpha_dot / 4
        else:
            # Otherwise, apply normal DC motor model.
            alpha_dotdot = -self.c0 * alpha_dot + duty * self.c1

        # Return the state derivative.
        return array([alpha_dot, alpha_dotdot])
