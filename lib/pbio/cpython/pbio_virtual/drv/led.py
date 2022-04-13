# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from typing import Callable, List, NamedTuple


class VirtualLed:
    class SetHsvEvent(NamedTuple):
        timestamp: int
        """
        The time when the event occurred as 32-bit unsigned microseconds.
        """
        h: int
        """
        The requested hue (0 - 359).
        """
        s: int
        """
        The requested saturation (0 - 100).
        """
        v: int
        """
        The requested value (0 - 100).
        """

    SetHsvCallback = Callable[[SetHsvEvent], None]
    Unsubscribe = Callable[[], None]

    _set_hsv_subscriptions: List[SetHsvCallback]

    def __init__(self) -> None:
        self._set_hsv_subscriptions = []

    def subscribe_set_hsv(self, callback: SetHsvCallback) -> Unsubscribe:
        """
        Subscribes to set HSV events.

        Args:
            callback:
                A function that will be called each time :meth:`on_set_hsv()` is called.

        Returns:
            A function that, when called, will unsubscribe from the events.
        """
        self._set_hsv_subscriptions.append(callback)
        return lambda: self._set_hsv_subscriptions.remove(callback)

    def on_set_hsv(self, *args) -> None:
        """
        This method called from ``pbdrv_led_virtual_set_hsv()``.
        """
        event = self.SetHsvEvent(*args)

        for callback in self._set_hsv_subscriptions:
            callback(event)
