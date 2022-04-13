# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from typing import NamedTuple

from ..drv.button import VirtualButtons
from ..drv.battery import VirtualBattery
from ..drv.led import VirtualLed
from . import DefaultPlatform


class RGB(NamedTuple):
    """
    An RGB value
    """

    r: int
    """
    red
    """
    g: int
    """
    green
    """
    b: int
    """
    blue
    """


class DefaultLed(VirtualLed):
    rgb = RGB(0, 0, 0)
    """
    The most recent receive RGB value.
    """

    def on_set_hsv(self, r: int, g: int, b: int) -> None:
        self.rgb = RGB(r, g, b)


class Platform(DefaultPlatform):
    def __init__(self):
        super().__init__()

        self.battery[-1] = VirtualBattery()
        self.button[-1] = VirtualButtons()
        self.led[0] = DefaultLed()
