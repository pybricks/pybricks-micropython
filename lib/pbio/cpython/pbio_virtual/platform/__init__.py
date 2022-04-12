# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


from typing import Sequence

from ..drv.battery import VirtualBattery
from ..drv.button import ButtonFlags


class DefaultPlatform:
    """
    Base class for virtual hub implementations.
    """

    def __init__(self):
        self.battery = VirtualBattery()

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

    @property
    def buttons(self) -> ButtonFlags:
        """
        Gets the buttons that are currently pressed.

        The virtual button driver uses this value.

        The default implementation returns a constant value of ``ButtonFlags(0)``.

        Returns:
            The button flags of the currently pressed buttons.
        """
        return ButtonFlags(0)

    @property
    def counter_count(self) -> Sequence[int]:
        """
        Gets the current counter count for each counter.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6

    @property
    def counter_abs_count(self) -> Sequence[int]:
        """
        Gets the current counter absolute count for each counter.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6

    @property
    def counter_abs_rate(self) -> Sequence[int]:
        """
        Gets the current counter rate for each counter in counts per second.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6
