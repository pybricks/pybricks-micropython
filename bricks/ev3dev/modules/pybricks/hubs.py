# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

from sys import stderr, exit

from .speaker import Speaker
from .display import Display

from hubs_c import EV3Brick as EV3Brick_c


class EV3Brick(EV3Brick_c):
    def __init__(self):
        try:
            # Initialize the EV3 speaker and display
            self.speaker = Speaker('EV3')
            self.display = Display('EV3')
        except Exception as exception:
            print("Pybricks is already running on this device. Exiting...",
                  file=stderr)
            exit(1)
