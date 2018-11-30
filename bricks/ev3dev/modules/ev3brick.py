# MIT License
#
# Copyright (c) 2017 Laurens Valk
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""LEGO MINDSTORMS EV3 Programmable Brick.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45500: Separate part (2013)

LEGO ID: 95646
"""

from sys import stderr
from builtins import print as builtinprint

from uev3dev._sound import _beep

# import those features of the EV3 brick that are already written in MicroPython-style C code.
from ev3brick_c import *
from timing import wait


def print(*args, **kwargs):
    """Print a message on the IDE terminal."""
    builtinprint(*args, file=stderr, **kwargs)


def beep(frequency=500, duration=100, volume=30):
    """Play a beep.

    Keyword Arguments:
        frequency {int} -- Frequency of the beep (Hz) (default: {500})
        duration {int} -- Duration of the beep (milliseconds) (default: {100})
        volume {int} -- Volume of the beep (0-100%) (default: {30})
    """
    _beep(frequency, duration, volume)


def beeps(number):
    """Play a number of beeps with a brief pause in between.

    Arguments:
        number {int} -- Number of beeps
    """
    for i in range(number):
        beep()
        wait(100)


def tune(frequencies_and_durations, volume=30):
    """Play a tune composed of beeps.

    Keyword Arguments:
        frequencies_and_durations {list} -- List of (frequency, duration) pairs
        volume {int} -- Volume of the tune (0-100%) (default: {30})
    """
    for (frequency, duration) in frequencies_and_durations:
        beep(frequency, duration, volume)
