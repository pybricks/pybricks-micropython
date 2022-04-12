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
