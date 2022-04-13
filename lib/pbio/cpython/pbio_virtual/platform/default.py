# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from typing import NamedTuple

from ..drv.button import VirtualButtons
from ..drv.battery import VirtualBattery
from ..drv.led import VirtualLed
from . import DefaultPlatform


class HSV(NamedTuple):
    """
    Hue, saturation, value tuple.
    """

    h: int
    """
    hue
    """
    s: int
    """
    saturation
    """
    v: int
    """
    value
    """


class DefaultLed(VirtualLed):
    hsv = HSV(0, 0, 0)
    """
    The most recently receive HSV value.
    """

    def on_set_hsv(self, timesamp: int, h: int, s: int, v: int) -> None:
        self.hsv = HSV(h, s, v)


class Platform(DefaultPlatform):
    def __init__(self):
        super().__init__()

        self.battery[-1] = VirtualBattery()
        self.button[-1] = VirtualButtons()
        self.led[0] = DefaultLed()
