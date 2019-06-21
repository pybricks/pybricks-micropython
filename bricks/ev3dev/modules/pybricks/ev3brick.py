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

from ev3brick_c import battery

from .speaker import Speaker
from .display import Display


try:
    # Initialize the EV3 speaker and display
    sound = Speaker('EV3')
    display = Display('EV3')
except Exception as exception:
    print("Pybricks is already running on this device. Exiting...", file=stderr)
    exit()

# The new light and button API can be enabled using:
# from ev3brick_c import light, buttons
# However, we use the following workaround to maintain backwards
# compatibility with the 1.0.0 API.
from ev3brick_c import light as newlight
from ev3brick_c import buttons as newbuttons


class CompatLight():
    def __call__(self, color):
        newlight.on(color)

    def off(self):
        newlight.off()

    def on(self, color, brightness=100):
        newlight.on(color, brightness)


class CompatButtons():
    def __call__(self):
        # this provides the old syntax
        return newbuttons.pressed()

    # The following are the actual (new) methods
    def pressed(self):
        return newbuttons.pressed()


light = CompatLight()
buttons = CompatButtons()
