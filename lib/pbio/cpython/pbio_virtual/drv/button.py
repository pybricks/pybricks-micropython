# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from enum import IntFlag


class ButtonFlags(IntFlag):
    """
    Hub buttons.

    These values match ``pbio_button_flags_t``.
    """

    LEFT_DOWN = 1 << 1
    DOWN = 1 << 2
    RIGHT_DOWN = 1 << 3
    LEFT = 1 << 4
    CENTER = 1 << 5
    RIGHT = 1 << 6
    LEFT_UP = 1 << 7
    UP = 1 << 8
    RIGHT_UP = 1 << 9


class VirtualButtons:
    pressed: ButtonFlags = ButtonFlags(0)
    """
    Provides the button flags returned by ``pbdrv_button_is_pressed()``.

    CPython code should write to this attribute to simulate the buttons currently
    pressed and the PBIO driver will read this attribute.
    """
