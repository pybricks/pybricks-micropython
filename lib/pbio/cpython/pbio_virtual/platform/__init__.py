# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


from typing import Dict

from ..drv.battery import VirtualBattery
from ..drv.button import VirtualButtons
from ..drv.counter import VirtualCounter
from ..drv.led import VirtualLed


class DefaultPlatform:
    """
    Base class for virtual hub implementations.
    """

    battery: VirtualBattery
    buttons: VirtualButtons
    counter: Dict[int, VirtualCounter]
    led: Dict[int, VirtualLed]

    def __init__(self):
        self.battery = VirtualBattery()
        self.buttons = VirtualButtons()
        self.counter = {}
        self.led = {}

    def on_event_poll(self) -> None:
        """
        This method is called during MICROPY_EVENT_POLL_HOOK in MicroPython.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_event_poll()``.
        """
        pass
