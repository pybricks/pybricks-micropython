# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from __future__ import annotations

import colorsys
import threading
import tkinter as tk
import turtle
from typing import NamedTuple, Optional

from ..drv.battery import VirtualBattery
from ..drv.button import ButtonFlags, VirtualButtons
from ..drv.clock import WallClock
from ..drv.counter import VirtualCounter
from ..drv.ioport import PortId, VirtualIOPort
from ..drv.led import VirtualLed
from ..drv.motor_driver import VirtualMotorDriver
from . import DefaultPlatform


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

    DUTY_SCALE_FACTOR = 0.1

    class State(NamedTuple):
        timestamp: int
        is_coasting: bool = True
        duty_cycle: float = 0
        count: float = 0
        rate: float = 0

    _turtle: turtle.Turtle
    _state: State
    _port: Optional[VirtualIOPort]
    _driver: Optional[VirtualMotorDriver]
    _counter: Optional[VirtualCounter]

    def __init__(self) -> None:
        self._turtle = turtle.Turtle(shape="arrow")
        self._turtle.penup()
        self._turtle.goto(-200, 0)
        self._turtle.resizemode("user")
        self._turtle.shapesize(3, 3)

        # TODO: get real timestamp
        self._state = Motor.State(timestamp=0)

        self._port = None
        self._driver = None
        self._counter = None

    def attach(
        self,
        port: VirtualIOPort,
        driver: VirtualMotorDriver,
        counter: VirtualCounter,
    ) -> None:
        # SPIKE medium motor
        # TODO: should fill all fields
        port._info.type_id = 48
        port._info.capability_flags = 0xF

        # REVISIT: if we need to detach the motor, we will need to save the
        # returned unsubscribe functions
        driver.subscribe_coast(self._on_coast)
        driver.subscribe_duty_cycle(self._on_duty_cycle)

        self._port = port
        self._driver = driver
        self._counter = counter

        # TODO: get real timestamp
        self._state = Motor.State(timestamp=0)
        self._apply_state()

    def _apply_state(self) -> None:
        self._counter.count = int(self._state.count)
        self._counter.abs_count = int(self._state.count % 360)
        self._counter.rate = int(self._state.rate)
        self._turtle.tiltangle(self._counter.abs_count)

    def _on_coast(self, event: VirtualMotorDriver.CoastEvent) -> None:
        count = self._state.count

        # integrate every 1 millisecond since last timestamp
        for _ in range(self._state.timestamp, event.timestamp, 1000):
            count += self._state.duty_cycle * self.DUTY_SCALE_FACTOR

        self._state = self._state._replace(
            timestamp=event.timestamp, is_coasting=True, duty_cycle=0, count=count
        )
        self._apply_state()

    def _on_duty_cycle(self, event: VirtualMotorDriver.DutyCycleEvent) -> None:
        count = self._state.count

        # integrate every 1 millisecond since last timestamp
        for _ in range(self._state.timestamp, event.timestamp, 1000):
            count += event.duty_cycle * self.DUTY_SCALE_FACTOR

        self._state = self._state._replace(
            timestamp=event.timestamp,
            is_coasting=False,
            duty_cycle=event.duty_cycle,
            count=count,
        )
        self._apply_state()


class Platform(DefaultPlatform):
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
        self._motor.attach(self.ioport[PortId.A], self.motor_driver[0], self.counter[0])

        # event hooks

        self._window_close_event = threading.Event()
        turtle.getcanvas().winfo_toplevel().protocol(
            "WM_DELETE_WINDOW", self._window_close_event.set
        )

    def on_event_poll(self) -> None:
        # send SystemExit to MicroPython runtime when window close button is clicked
        if self._window_close_event.is_set():
            raise SystemExit

        turtle.update()

        root = turtle.getcanvas().winfo_toplevel()

        while root.dooneevent(tk._tkinter.DONT_WAIT):
            pass
