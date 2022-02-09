# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

import turtle
import tkinter as tk

from . import VirtualHub as BaseVirtualHub


def do_events() -> None:
    """
    Processes any pending TCL events without blocking wait.
    """
    root = turtle.getcanvas().winfo_toplevel()
    while root.dooneevent(tk._tkinter.DONT_WAIT):
        pass


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


def draw_light(*color) -> None:
    """
    Draws the hub status light.

    Args:
        color: Any acceptable arg for ``turtle.color(...)``.
    """
    turtle.up()
    turtle.goto(0, -75)
    turtle.down()
    turtle.color(*color)
    turtle.begin_fill()
    turtle.circle(15)
    turtle.end_fill()


class VirtualHub(BaseVirtualHub):
    """
    This is a ``VirtualHub`` implementation that uses turtle graphics to draw
    the virtual hub.
    """

    def __init__(self) -> None:
        super().__init__()

        # we are using turtle just for drawing, so disable animations, etc.
        turtle.hideturtle()
        turtle.delay(0)

        # draw initial hub
        draw_hub()
        draw_light("black")

    def on_event_poll(self) -> None:
        do_events()
