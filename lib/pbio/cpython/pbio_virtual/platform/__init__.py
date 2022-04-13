# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


import abc
from typing import Dict

from ..drv.battery import VirtualBattery
from ..drv.button import VirtualButtons
from ..drv.counter import VirtualCounter
from ..drv.led import VirtualLed
from ..drv.motor_driver import VirtualMotorDriver


class DefaultPlatform(abc.ABC):
    """
    Base class for virtual hub implementations.
    """

    battery: Dict[int, VirtualBattery]
    """
    The battery driver components.

    PBIO currently only supports a single battery instance, so overriding
    classes should assign ``battery[-1] = VirtualBattery()`` during init.
    """

    button: Dict[int, VirtualButtons]
    """
    The button driver components.

    PBIO currently only supports a single button instance, so overriding
    classes should assign ``button[-1] = VirtualButton()`` during init.
    """

    counter: Dict[int, VirtualCounter]
    """
    The counter driver components.

    Overriding classes should assign ``counter[<id>] = VirtualCounter()`` for
    each counter device during init.
    """

    led: Dict[int, VirtualLed]
    """
    The LED driver components.

    Overriding classes should assign ``led[<id>] = VirtualLed()`` for
    each LED device during init.
    """

    motor_driver: Dict[int, VirtualMotorDriver]
    """
    The motor driver driver components.

    Overriding classes should assign ``motor_driver[<id>] = VirtualMotorDriver()``
    for each motor driver device during init.
    """

    def __init__(self):
        self.battery = {}
        self.button = {}
        self.counter = {}
        self.led = {}
        self.motor_driver = {}

    def on_event_poll(self) -> None:
        """
        This method is called during MICROPY_EVENT_POLL_HOOK in MicroPython.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_event_poll()``.
        """
        pass
