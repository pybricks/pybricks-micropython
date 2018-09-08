"""This module is for the LEGO BOOST Move Hub."""

import _pbport

from pblight import Light
from pbsensor import TouchSensor


class Port(_pbport.Port):
    """The ports on the LEGO BOOST Move Hub"""
    HUB = -1
    """The Move Hub itself"""

    A = ord('A')
    """Built-in motor 'A'"""

    B = ord('B')
    """Built-in motor 'B'"""

    C = ord('C')
    """I/O port 'C'"""

    D = ord('D')
    """I/O port 'D'"""

button = TouchSensor(Port.HUB)
"""The button on the Move Hub"""

light = Light(Port.HUB)
"""The light on the Move Hub"""
