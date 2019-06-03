# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""LEGO MINDSTORMS EV3 Programmable Brick.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45500: Separate part (2013)

LEGO ID: 95646
"""

from sys import stderr, exit

# import those features of the EV3 Brick that are already written in MicroPython-style C code.
from ev3brick_c import buttons, battery

from .speaker import Speaker
from .display import Display


try:
    # Initialize the EV3 speaker and display
    sound = Speaker('EV3')
    display = Display('EV3')
except Exception as exception:
    print("Pybricks is already running on this device. Exiting...", file=stderr)
    exit()

# The new light API can be enabled using:
# from ev3brick_c import light
# However, we use the following workaround to maintain backwards
# compatibility with the 1.0.0 API.
from ev3brick_c import light as newlight

class CompatLight():
    def __call__(self, color):
        newlight.color(color)
    
    def off(self):
        newlight.off()

    def color(self, color):
        newlight.color(color)

    def brightness(self, red, green, blue):
        newlight.brightness(red, green, blue)

light = CompatLight()
