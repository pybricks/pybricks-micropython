# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Pybricks robotics module."""


from robotics_c import DriveBase as DriveBase_c
from tools import wait


class DriveBase(DriveBase_c):
    # Add Legacy EV3 MicroPython 1.0 function
    def drive_time(self, speed, steering, time):
        self.drive(speed, steering)
        wait(time)
        self.stop()
