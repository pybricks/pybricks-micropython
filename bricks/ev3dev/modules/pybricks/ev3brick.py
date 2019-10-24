# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Backwards compatibility module for pybricks.ev3brick"""


# Import the new EV3 Brick class
from .hubs import EV3Brick

# Create an instance of an EV3 Brick
_brickobj = EV3Brick()

# Map the instance attributes to a flattened module
# to make it work like the MicroPython 1.0 API.

sound = _brickobj.speaker

display = _brickobj.display

battery = _brickobj.battery

# Map new class methods to old functions


def light(color):
    _brickobj.light.on(color)


def buttons():
    return _brickobj.buttons.pressed()
