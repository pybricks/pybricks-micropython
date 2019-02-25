# SPDX-License-Identifier: MIT
# Copyright (c) 2018 Laurens Valk

"""Pybricks robotics module."""
from pybricks.parameters import Stop
from pybricks.tools import wait
from math import pi


class DriveBase():
    def __init__(self, left_motor, right_motor, wheel_diameter, axle_track):
        self.left_motor = left_motor
        self.right_motor = right_motor
        self.wheel_diameter = wheel_diameter
        self.axle_track = axle_track

    def drive(self, speed, steering):
        speedsum = speed/self.wheel_diameter*(720/pi)
        speeddif = 2*self.axle_track/self.wheel_diameter*steering
        self.left_motor.run((speedsum+speeddif)/2)
        self.right_motor.run((speedsum-speeddif)/2)

    def drive_time(self, speed, steering, time):
        self.drive(speed, steering)
        wait(time)
        self.stop()

    def stop(self, stop_type=Stop.coast):
        self.left_motor.stop(stop_type)
        self.right_motor.stop(stop_type)
