# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from typing import Callable, List, NamedTuple


class VirtualMotorDriver:
    """
    Virtual motor driver chip implementation.
    """

    class CoastEvent(NamedTuple):
        timestamp: int
        """
        The time when the event occurred as 32-bit unsigned microseconds.
        """

    class DutyCycleEvent(NamedTuple):
        timestamp: int
        """
        The time when the event occurred as 32-bit unsigned microseconds.
        """
        duty_cycle: float
        """
        The requested duty cycle (-1.0 to 1.0).
        """

    CoastCallback = Callable[[CoastEvent], None]
    DutyCycleCallback = Callable[[DutyCycleEvent], None]
    Unsubscribe = Callable[[], None]

    _coast_subscriptions: List[CoastCallback]
    _duty_cycle_subscriptions: List[DutyCycleCallback]

    def __init__(self) -> None:
        self._coast_subscriptions = []
        self._duty_cycle_subscriptions = []

    def subscribe_coast(self, callback: CoastCallback) -> Unsubscribe:
        """
        Subscribes to coast events.

        Args:
            callback: A function that will be called each time :meth:`on_coast()` is called.

        Returns:
            A function that, when called, will unsubscribe from the events.
        """
        self._coast_subscriptions.append(callback)
        return lambda: self._coast_subscriptions.remove(callback)

    def on_coast(self, *args) -> None:
        """
        Called when ``pbdrv_motor_driver_coast()`` is called.
        """
        event = self.CoastEvent(*args)

        for callback in self._coast_subscriptions:
            callback(event)

    def subscribe_duty_cycle(self, callback: DutyCycleCallback) -> Unsubscribe:
        """
        Subscribes to duty cycle events.

        Args:
            callback:
                A function that will be called each time
                :meth:`on_set_duty_cycle()` is called.

        Returns:
            A function that, when called, will unsubscribe from the events.
        """
        self._duty_cycle_subscriptions.append(callback)
        return lambda: self._duty_cycle_subscriptions.remove(callback)

    def on_set_duty_cycle(self, *args) -> None:
        """
        Called when ``pbdrv_motor_driver_set_duty_cycle()`` is called.
        """
        event = self.DutyCycleEvent(*args)

        for callback in self._duty_cycle_subscriptions:
            callback(event)
