"""Lights can be built into programmable bricks or can be plugged into I/O
ports."""

from collections import namedtuple
from enum import Enum
from typing import Union

from _pbport import Port


class Color(Enum):
    """Color names"""

    red = 1
    """The light will be red"""

    orange = 2
    """The light will be orange"""

    yellow = 3
    """The light will be yellow"""

    green = 4
    """The light will be green"""

    blue = 5
    """The light will be blue"""

    purple = 6
    """The light will be purple"""

    white = 7
    """The light will be white"""

RGB = namedtuple('RGB', 'r g b')
"""Red, green, blue value"""


class Pattern(Enum):
    """Blink patterns"""

    flash = 1
    """The light will alternate between *color1* and *color2* once per
     second"""

    fade = 2
    """The light will fade gradually from *color1* to *color2* and back once
     per second"""


class Light():
    """Class representing a light"""
    def __init__(self, port: Port):
        """Creates a new instance

        Args:
            port: the port the light is connected to
        """
        self._port = port

    def on(self, color: Union[Color, RGB, None]) -> None:
        """Turns the light on with the specified color

        Args:
            color: the color
        """
        pass

    def off(self) -> None:
        pass

    def blink(self, color1: Union[Color, RGB, None],
              color2: Union[Color, RGB, None], pattern: Pattern) -> None:
        """Blinks the light using the specified pattern

        Args:
            color1: the first color in the pattern
            color2: the second color in the pattern
            pattern: the pattern
        """
        pass

    @property
    def has_red(self) -> bool:
        """Checks if the light can be red

        Returns:
            ``True`` if the light can be red
        """
        return False

    @property
    def has_green(self) -> bool:
        """Checks if the light can be green

        Returns:
            ``True`` if the light can be green
        """
        return False

    @property
    def has_blue(self) -> bool:
        """Checks if the light can be blue

        Returns:
            ``True`` if the light can be blue
        """
        return False
