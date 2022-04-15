# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from __future__ import annotations

import colorsys
from functools import wraps
import threading
import time
import tkinter as tk
import turtle
from typing import Callable, NamedTuple, Optional, Tuple


from ..drv.battery import VirtualBattery
from ..drv.button import ButtonFlags, VirtualButtons
from ..drv.clock import WallClock, VirtualClock
from ..drv.counter import VirtualCounter
from ..drv.ioport import IODeviceTypeId, PortId, VirtualIOPort
from ..drv.led import VirtualLed
from ..drv.motor_driver import VirtualMotorDriver
from . import VirtualPlatform


class fps:
    """
    Decorator to rate-limit UI updates.
    """

    _interval: int
    """
    The interval between updates in nanoseconds.
    """

    _last: int
    """
    The time of the last update in nanoseconds.
    """

    def __init__(self, fps: float) -> None:
        """
        Args:
            fps: The rate in frames per seconds.
        """
        self._interval = 1000000000 // fps
        self._last = time.monotonic_ns()

    def __call__(self, func: Callable):
        @wraps(func)
        def wrapper(*args, **kwargs):
            now = time.monotonic_ns()
            delta = now - self._last

            if delta > self._interval:
                self._last = now
                return func(*args, **kwargs)

        return wrapper


def draw_hub() -> None:
    """
    Draws the non-interactive, non-animated parts of the hub.
    """
    turtle.up()
    turtle.goto(-100, -100)
    turtle.down()
    turtle.color("black")
    turtle.goto(-100, 100)
    turtle.goto(100, 100)
    turtle.goto(100, -100)
    turtle.goto(-100, -100)


class StatusLightButton:
    """
    A status light/button combo like the one on SPIKE Prime.
    """

    _turtle: turtle.Turtle

    def __init__(self, light: VirtualLed, button: VirtualButtons) -> None:
        self._turtle = turtle.Turtle(shape="circle")
        self._turtle.shapesize(2)
        self._turtle.penup()
        self._turtle.goto(0, -70)

        light.subscribe_set_hsv(
            lambda event: self._turtle.color(
                colorsys.hsv_to_rgb(event.h / 360, event.s / 100, event.v / 100)
            )
        )

        def press_center_button():
            button.pressed |= ButtonFlags.CENTER

        self._turtle.onclick(lambda x, y: press_center_button())

        def release_center_button():
            button.pressed &= ~ButtonFlags.CENTER

        self._turtle.onrelease(lambda x, y: release_center_button())


class Motor:
    """
    A fake motor. With turtle graphics.
    """

    # degrees per second at 100% duty cycle
    MAX_SPEED = 1600

    class State(NamedTuple):
        timestamp: int
        is_coasting: bool = True
        duty_cycle: float = 0
        count: float = 0
        rate: float = 0

    _turtle: turtle.Turtle
    _state: Optional[State]
    _port: Optional[VirtualIOPort]
    _driver: Optional[VirtualMotorDriver]
    _counter: Optional[VirtualCounter]

    def __init__(self) -> None:
        self._turtle = turtle.Turtle(shape="arrow")
        self._turtle.penup()
        self._turtle.goto(-200, -50)
        self._turtle.pendown()
        self._turtle.circle(50)
        self._turtle.penup()
        self._turtle.goto(-200, 0)
        self._turtle.resizemode("user")
        self._turtle.shapesize(3, 3)

        self._state = None
        self._port = None
        self._driver = None
        self._counter = None

    def attach(
        self,
        port: VirtualIOPort,
        driver: VirtualMotorDriver,
        counter: VirtualCounter,
        clock: VirtualClock,
        platform: VirtualPlatform,
    ) -> None:
        port.motor_type_id = IODeviceTypeId.SPIKE_M_MOTOR

        # REVISIT: if we need to detach the motor, we will need to save the
        # returned unsubscribe functions
        driver.subscribe_coast(self._on_coast)
        driver.subscribe_duty_cycle(self._on_duty_cycle)
        platform.subscribe_poll(self._on_poll)

        self._port = port
        self._driver = driver
        self._counter = counter

        self._state = Motor.State(clock.nanoseconds // 1000)
        self._apply_state()

    def _apply_state(self) -> None:
        self._counter.count = int(self._state.count)
        self._counter.abs_count = int(self._state.count % 360)
        self._counter.rate = int(self._state.rate)
        # positive is clockwise in Pybricks
        self._turtle.tiltangle(-self._counter.abs_count)

    def _calc_new_output(self, new_timestamp: int) -> Tuple(int, int):
        delta = new_timestamp - self._state.timestamp
        new_count = self._state.count + self._state.duty_cycle * delta * self.MAX_SPEED / 1000000
        new_rate = self._state.duty_cycle * self.MAX_SPEED

        return new_count, new_rate

    def _on_coast(self, event: VirtualMotorDriver.CoastEvent) -> None:
        new_count, new_rate = self._calc_new_output(event.timestamp)

        self._state = self._state._replace(
            timestamp=event.timestamp,
            is_coasting=True,
            duty_cycle=0,
            count=new_count,
            rate=new_rate,
        )

        self._apply_state()

    def _on_duty_cycle(self, event: VirtualMotorDriver.DutyCycleEvent) -> None:
        new_count, new_rate = self._calc_new_output(event.timestamp)

        self._state = self._state._replace(
            timestamp=event.timestamp,
            is_coasting=False,
            duty_cycle=event.duty_cycle,
            count=new_count,
            rate=new_rate,
        )

        self._apply_state()

    def _on_poll(self, event: VirtualPlatform.PollEvent) -> None:
        new_count, new_rate = self._calc_new_output(event.timestamp)

        self._state = self._state._replace(
            timestamp=event.timestamp, count=new_count, rate=new_rate
        )

        self._apply_state()


class Platform(VirtualPlatform):
    """
    This is a ``Platform`` implementation that uses turtle graphics to draw
    the virtual hub.
    """

    def __init__(self) -> None:
        super().__init__()

        # we are using turtle just for drawing, so disable animations, etc.
        turtle.hideturtle()
        turtle.tracer(0, 0)

        # draw initial hub
        draw_hub()

        # rate-limited UI updates
        self.subscribe_poll(self._update_ui)

        # drivers
        self.battery[-1] = VirtualBattery()
        self.button[-1] = VirtualButtons()
        self.clock[-1] = WallClock()
        for i in range(6):
            self.counter[i] = VirtualCounter()
        for p in range(PortId.A, PortId.F + 1):
            self.ioport[p] = VirtualIOPort(p)
        self.led[0] = VirtualLed()
        for i in range(6):
            self.motor_driver[i] = VirtualMotorDriver()

        # built-in devices
        self._status_light = StatusLightButton(self.led[0], self.button[-1])

        # attached I/O devices
        self._motor = Motor()
        self._motor.attach(
            self.ioport[PortId.A],
            self.motor_driver[0],
            self.counter[0],
            self.clock[-1],
            self,
        )

        # exit when window close button is clicked

        self._window_close_event = threading.Event()
        turtle.getcanvas().winfo_toplevel().protocol(
            "WM_DELETE_WINDOW", self._window_close_event.set
        )

        def on_poll(event):
            if self._window_close_event.is_set():
                raise SystemExit

        self.subscribe_poll(on_poll)

    @fps(30)
    def _update_ui(self, event: VirtualPlatform.PollEvent) -> None:
        turtle.update()

        root = turtle.getcanvas().winfo_toplevel()

        while root.dooneevent(tk._tkinter.DONT_WAIT):
            pass
