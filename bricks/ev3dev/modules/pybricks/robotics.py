# The MIT License (MIT)
#
# Copyright (c) 2018 Laurens Valk
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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
