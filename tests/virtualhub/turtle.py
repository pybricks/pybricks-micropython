# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from __future__ import annotations

import turtle
import tkinter as tk
from typing import Tuple, Union

from . import Buttons, VirtualHub as BaseVirtualHub


# Counterpart to turtle.onscreenclick() since the standard library does not
# provide this function.
def _onscreenrelease(self, fun, num=1, add=None):
    """Bind fun to mouse-release event on canvas.
    fun must be a function with two arguments, the coordinates
    of the clicked point on the canvas.
    num, the number of the mouse-button defaults to 1
    If a turtle is clicked, first _onrelease-event will be performed,
    then _onscreenrelease-event.
    """
    if fun is None:
        self.cv.unbind("<Button%s-ButtonRelease>" % num)
    else:

        def eventfun(event):
            x, y = (
                self.cv.canvasx(event.x) / self.xscale,
                -self.cv.canvasy(event.y) / self.yscale,
            )
            fun(x, y)

        self.cv.bind("<Button%s-ButtonRelease>" % num, eventfun, add)


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


def draw_light(color: Union[str, Tuple[int, int, int]]) -> None:
    """
    Draws the hub status light.

    Args:
        color: The name of a color or a tuple of RGB values.
    """
    turtle.up()
    turtle.goto(0, -75)
    turtle.down()
    turtle.color("black", color)
    turtle.begin_fill()
    turtle.circle(15)
    turtle.end_fill()


def on_click(x: float, y: float, mouse_down: bool, hub: VirtualHub) -> None:
    if -15 <= x <= 15 and -90 <= y <= -60:
        # mouse click is within bounds of light/button
        if mouse_down:
            hub._buttons |= Buttons.CENTER
        else:
            hub._buttons &= ~Buttons.CENTER


class VirtualHub(BaseVirtualHub):
    """
    This is a ``VirtualHub`` implementation that uses turtle graphics to draw
    the virtual hub.
    """

    def __init__(self) -> None:
        super().__init__()

        self._buttons = Buttons(0)

        # we are using turtle just for drawing, so disable animations, etc.
        turtle.hideturtle()
        turtle.delay(0)
        turtle.tracer(0, 0)
        turtle.colormode(255)

        # event hooks
        turtle.onscreenclick(lambda x, y: on_click(x, y, True, self))
        _onscreenrelease(turtle.Turtle._screen, lambda x, y: on_click(x, y, False, self))

        # draw initial hub
        draw_hub()
        draw_light("black")

    def on_event_poll(self) -> None:
        do_events()

    def on_light(self, id: int, r: int, g: int, b: int) -> None:
        if id != 0:
            return

        draw_light((r, g, b))

    @property
    def buttons(self) -> Buttons:
        return self._buttons
