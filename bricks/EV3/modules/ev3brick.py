"""LEGO MINDSTORMS EV3 Programmable Brick.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45500: Separate part (2013)

LEGO ID: 95646
"""

from sys import stderr
from builtins import print as builtinprint

# import those features of the EV3 brick that are already written in MicroPython-style C code.
from ev3brick_c import *

from uev3dev._sound import _beep


def print(*args, **kwargs):
    """Print a message on the IDE terminal."""
    builtinprint(*args, file=stderr, **kwargs)


# TODO: Delete dummy API below or implement it.

def battery_voltage():
    """Get the battery voltage.

    Returns:
        float -- Battery voltage (Volt)

    """
    pass


def battery_low():
    """Check if the battery is low.

    Returns:
        bool -- True if the battery voltage is low. False if it's not.

    """
    pass


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


def display_clear():
    """Clear anything on the display."""
    pass


def display_text(text, x, y, size=2):
    """Display text. The x and y coordinate specify the top left of the first character.

    Arguments:
        text {str} -- Text to display.
        x {int} -- x location, counted from the left of the first character (0-177)
        y {int} -- y location, counted from the top of the first character (0-128)
        size {int} -- Font size: SMALL, MEDIUM, or LARGE
    """
    pass


def display_text_line(text, line):
    """Display text at the given line number and erase any previous text on this line.

    Arguments:
        line {int} -- Line number (0-6)
        text {str} -- Text to display.
    """
    pass
