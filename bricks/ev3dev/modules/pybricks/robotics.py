# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Pybricks robotics module."""

from robotics_c import DriveBase as CompatDriveBase

from pybricks.tools import wait
from pybricks.parameters import Stop


class DriveBase(CompatDriveBase):
    """DriveBase class backwards-compatible with 1.0 EV3 release."""

    def stop(self, stop_type=Stop.COAST):
        """stop method backwards-compatible with 1.0 EV3 release."""
        if stop_type == Stop.COAST:
            super().stop()
        elif stop_type == Stop.BRAKE:
            super().stop()
            self.left.brake()
            self.right.brake()
        elif stop_type == Stop.HOLD:
            self.straight(0)
        else:
            raise ValueError("Invalid Argument")

    def drive_time(self, speed, steering, time):
        """drive_time method backwards-compatible with 1.0 EV3 release."""
        self.drive(speed, steering)
        wait(time)
        self.stop()
