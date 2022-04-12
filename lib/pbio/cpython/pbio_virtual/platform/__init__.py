# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


from typing import Dict
from ..drv.battery import VirtualBattery
from ..drv.button import VirtualButtons
from ..drv.counter import VirtualCounter


class DefaultPlatform:
    """
    Base class for virtual hub implementations.
    """

    battery: VirtualBattery
    buttons: VirtualButtons
    counter: Dict[int, VirtualCounter]

    def __init__(self):
        self.battery = VirtualBattery()
        self.buttons = VirtualButtons()
        self.counter = {}

    def on_event_poll(self) -> None:
        """
        This method is called during MICROPY_EVENT_POLL_HOOK in MicroPython.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_event_poll()``.
        """
        pass

    def on_light(self, id: int, r: int, g: int, b: int) -> None:
        """
        This method is from the ``led_virtual`` driver when the color of the
        light is set.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_light()``.

        Args:
            id: The light identifier (0-based index).
            r: The requested red value (0 - 255).
            g: The requested green value (0 - 255).
            b: The requested blue value (0 - 255).
        """
        pass
