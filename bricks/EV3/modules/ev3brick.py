"""LEGO MINDSTORMS EV3 Programmable Brick.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45500: Separate part (2013)

LEGO ID: 95646
"""

# import those features of the EV3 brick that are already written in MicroPython-style C code.
from ev3brick_c import *
from ev3devio import write_int
from sys import stderr


def terminal(*args, **kwargs):
    """Print a message on the terminal."""
    print(*args, file=stderr, **kwargs)


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


def tone(frequency, duration, volume=100):
    """Play a tone at the specified frequency, duration, and volume.

    Arguments:
        frequency {int} -- Frequency of the tone (Herz)
        duration {float} -- Duration of the tone (seconds)

    Keyword Arguments:
        volume {int} -- Volume percentage from 0-100. (default: {100})
    """
    pass


def beep(beeps=1):
    """Play one or more beeps.

    Keyword Arguments:
        beeps {int} -- Number of subsequent short beeps (default: {1})
    """
    for i in range(beeps):
        tone(800, 0.05, 100)
        if beeps > 1:
            sleep(0.025)
    pass


# TODO: make consistent with HUB light API
# TODO: Add pattern, consistent with Hubs

led_left_green = open('/sys/class/leds/led0:green:brick-status/brightness', 'w')
led_left_red = open('/sys/class/leds/led0:red:brick-status/brightness', 'w')
led_right_green = open('/sys/class/leds/led1:green:brick-status/brightness', 'w')
led_right_red = open('/sys/class/leds/led1:red:brick-status/brightness', 'w')


def light(color):
    """Set the EV3 brick LED light to the specified color.

    Arguments:
        color {color} -- Color.Red, Color.Green, Color.Orange, or None
    """
    if color == Color.red:
        r = 255
        g = 0
    elif color == Color.green:
        r = 0
        g = 255
    elif color == Color.orange:
        r = 255
        g = 255
    else:
        r = 0
        g = 0
    write_int(led_left_green, g)
    write_int(led_left_red, r)
    write_int(led_right_green, g)
    write_int(led_right_red, r)


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
