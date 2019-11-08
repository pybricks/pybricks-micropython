# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Backwards compatibility module for pybricks.ev3brick"""

from sys import stderr, exit

from .speaker import Speaker
from .display import Display

# Import the new EV3 Brick class
from .hubs import EV3Brick

# Create an instance of an EV3 Brick
_brickobj = EV3Brick()

# Use EV3 MicroPython 1.0 sound/display implementation

try:
    # Initialize the EV3 speaker and display
    sound = Speaker('EV3')
    display = Display('EV3')
except Exception as exception:
    print("Pybricks is already running on this device. Exiting...",
            file=stderr)
    exit(1)

battery = _brickobj.battery

# Map new class methods to old functions


def light(color):
    _brickobj.light.on(color)


def buttons():
    return _brickobj.buttons.pressed()
