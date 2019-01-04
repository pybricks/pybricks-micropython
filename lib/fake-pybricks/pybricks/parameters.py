"""Constant parameters/arguments for the Pybricks API."""

from enum import Enum


class Color(Enum):
    """Light or surface color.

    .. data:: black
    .. data:: blue
    .. data:: green
    .. data:: yellow
    .. data:: red
    .. data:: white
    .. data:: brown
    .. data:: orange
    .. data:: purple
    """

    black = 1
    blue = 2
    green = 3
    yellow = 4
    red = 5
    white = 6
    brown = 7
    orange = 8
    purple = 9


class Port(Enum):
    """Port on the intelligent brick.

    Motor ports:

    .. data:: A
    .. data:: B
    .. data:: C
    .. data:: D

    Sensor ports:

    .. data:: S1
    .. data:: S2
    .. data:: S3
    .. data:: S4
    """

    # Generic motor/sensor ports
    A = ord('A')
    B = ord('B')
    C = ord('C')
    D = ord('D')

    # NXT/EV3 sensor ports
    S1 = ord('1')
    S2 = ord('2')
    S3 = ord('3')
    S4 = ord('4')


class Stop(Enum):
    """Action after the motor stops: coast, brake, or hold.

    .. data:: coast

        Let the motor move freely.

    .. data:: brake

        Passively resist small external forces.

    .. data:: hold

        Keep controlling the motor to hold it at the commanded angle.

    The stop type defines the resistance to motion after coming to standstill:

    +-----------+-------------+------------------------------------------+
    |Parameter  | Resistance  | Physical meaning                         |
    +===========+=============+==========================================+
    |Stop.coast | low         | Friction                                 |
    +-----------+-------------+------------------------------------------+
    |Stop.brake | medium      | Friction + Torque opposite to motion     |
    +-----------+-------------+------------------------------------------+
    |Stop.hold  | high        | Friction + Torque to hold commanded angle|
    +-----------+-------------+------------------------------------------+

    """

    coast = 0
    brake = 1
    hold = 2


class Direction():
    """Rotational direction for positive speed values: clockwise or counterclockwise.

    .. data:: clockwise

        A positive speed value should make the motor move clockwise.

    .. data:: counterclockwise

        A positive speed value should make the motor move counterclockwise.

    For all motors, this is defined when looking at the shaft, just like looking at a clock.

    For NXT or EV3 motors, make sure to look at the motor with the red/orange shaft to the lower right.

    +----------------------------+-------------------+-----------------+
    | Parameter                  | Positive speed    | Negative speed  |
    +============================+===================+=================+
    | Direction.clockwise        | clockwise         | counterclockwise|
    +----------------------------+-------------------+-----------------+
    | Direction.counterclockwise | counterclockwise  | clockwise       |
    +----------------------------+-------------------+-----------------+

    ::


         Medium EV3 Motor:


         counterclockwise          clockwise
               ____                 _____
              /                          \\
             /       _____________        \\
            /       /              \       \\
            |      |        _       |       |
            |      |     __| |__    |       |
            v      |    |__ o __|   |       v
                   |       |_|      |
                   |                |
                    \______________/


         Large EV3 motor:

              ________
             /         \        ___    ___
            _|          \      /          \\
            |             ----/------      \\
            counterclockwise  |    __\__    |  clockwise
              \__________     v  /      \   v
                          -------|  +   |
                                 \_____/


    """

    clockwise = 1
    counterclockwise = -1


class Button(Enum):
    """Buttons on a brick or remote:

    .. data:: left_down
    .. data:: down
    .. data:: right_down
    .. data:: left
    .. data:: center
    .. data:: right
    .. data:: left_up
    .. data:: up
    .. data:: beacon
    .. data:: right_up

    +-----------+----------+-----------+
    |           |          |           |
    | left_up   |up/beacon | right_up  |
    |           |          |           |
    +-----------+----------+-----------+
    |           |          |           |
    | left      |  center  | right     |
    |           |          |           |
    +-----------+----------+-----------+
    |           |          |           |
    | left_down |   down   | right_down|
    |           |          |           |
    +-----------+----------+-----------+
    """

    left_down = 1
    down = 2
    right_down = 3
    left = 4
    center = 5
    right = 6
    left_up = 7
    up = 8
    beacon = 8
    right_up = 9


class Pattern():
    """Standard light patterns."""

    solid = [(100, 1000)]
    """The light stays on."""

    blink1 = [(100, 500), (0, 500)]
    """The light blinks once per second: It stays on for 500 ms and then stays off for 500 ms."""

    blink2 = [(100, 250), (0, 250)]
    """The light blinks twice per second: It stays on for 250 ms and then stays off for 250 ms."""

    blink3 = [(100, 167), (0, 167)]
    """The light blinks three times per second: It stays on for 167 ms and then stays off for 167 ms."""

    glow = [] # TODO
    """The light gradually turns on and then off again, as if glowing. (TODO)"""

    ev3 = [(100, 100), (0, 100), (100, 100), (0, 700)] # TODO: Get the true values
    """The default pattern of a running EV3-G program."""


class Align():
    """Alignment of an image on a screen. TODO: use same logic as buttons??

    .. data:: center
    """
    bottom_left = 1
    bottom = 2
    bottom_right = 3
    left = 4
    center = 5
    right = 6
    top_left = 7
    top = 8
    top_right = 9


class Image():
    """Paths to standard EV3 images.

    .. data:: up
    """
    up = '/path/to/up'
