# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


import abc
from typing import Callable, Dict, List, NamedTuple


from ..drv.battery import VirtualBattery
from ..drv.button import VirtualButtons
from ..drv.clock import VirtualClock
from ..drv.counter import VirtualCounter
from ..drv.ioport import PortId, VirtualIOPort
from ..drv.led import VirtualLed
from ..drv.motor_driver import VirtualMotorDriver


class VirtualPlatform(abc.ABC):
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

    clock: Dict[int, VirtualClock]
    """
    The clock driver component.

    PBIO requires a single clock instance, so overriding classes must assign
    ``clock[-1] = VirtualClock()`` during init.
    """

    counter: Dict[int, VirtualCounter]
    """
    The counter driver components.

    Overriding classes should assign ``counter[<id>] = VirtualCounter()`` for
    each counter device during init.
    """

    ioport: Dict[PortId, VirtualIOPort]
    """
    The I/O port driver components.

    Overriding classes should assign ``ioport[<port>] = VirtualIOPort(<port>)`` for
    each I/O port during init.
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

    class PollEvent(NamedTuple):
        timestamp: int
        """
        The time when the event occurred as 32-bit unsigned microseconds.
        """

    PollCallback = Callable[[PollEvent], None]
    Unsubscribe = Callable[[], None]

    _poll_subscriptions: List[PollCallback]

    def __init__(self):
        self.battery = {}
        self.button = {}
        self.clock = {}
        self.counter = {}
        self.ioport = {}
        self.led = {}
        self.motor_driver = {}

        self._poll_subscriptions = []

    def subscribe_poll(self, callback: PollCallback) -> Unsubscribe:
        """
        Subscribes to poll events.

        Args:
            callback:
                A function that will be called each time :meth:`on_poll()` is called.

        Returns:
            A function that, when called, will unsubscribe from the events.
        """
        self._poll_subscriptions.append(callback)
        return lambda: self._poll_subscriptions.remove(callback)

    def on_poll(self, *args) -> None:
        """
        This method is called when ``pbdrv_virtual_platform_poll()`` is called.
        """
        event = self.PollEvent(*args)

        for callback in self._poll_subscriptions:
            callback(event)
