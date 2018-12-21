"""Constant parameters/arguments for the Pybricks API."""

from enum import Enum


class Color(Enum):
    """Light or surface color.

    black, blue, green, yellow, red, white, brown, orange, or purple.
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
    """Port on the intelligent brick."""

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

    This defines the resistance to motion after coming to standstill:

               | Resistance  | Physical meaning
    ---------------------------------------------------------
    Stop.coast | low         | Friction
    Stop.brake | medium      | Friction + Torque opposite to speed
    Stop.hold  | high        | Friction + Torque to maintain fixed angle

    """

    coast = 0
    brake = 1
    hold = 2


class Direction(Enum):
    """Direction for positive speed values: clockwise or counterclockwise.

    For all motors, this is defined when looking at the shaft, just like looking at a clock.

    For NXT or EV3 motors, make sure to look at the motor with the red/orange shaft to the lower right.

                               | Positive speed    | Negative speed
    ---------------------------------------------------------
    Direction.clockwise        | clockwise         | counterclockwise
    Direction.counterclockwise | counterclockwise  | clockwise

    """

    clockwise = 1
    counterclockwise = -1


#
#  Powered Up Motor:
#
#
#  counterclockwise          clockwise
#        ____                 _____
#       /                          \
#      /       _____________        \
#     /       /              \       \
#     |      |        _       |       |
#     |      |     __| |__    |       |
#     v      |    |__ o __|   |       v
#            |       |_|      |
#            |                |
#             \______________/
#
#
#  NXT and EV3 motors:
#
#       ________
#      /         \        ___    ___
#     _|          \      /          \
#     |             ----/------      \
#     counterclockwise  |    __\__    |  clockwise
#       \__________     v  /      \   v
#                   -------|  +   |
#                          \_____/
#


class Completion(Enum):
    """How the motor action should complete: foreground or background.

                            | Effect
    ---------------------------------------------------------
    Completion.foreground   | Motor moves in foreground: Your script waits for the motor action to complete.
    Completion.background   | Motor moves in background: Your script continues immediately while the motor is still moving.

    Note: A motor can only do one thing at the time. If a motor is running in
    the background and you give it a new command, only the new command applies.

    """

    foreground = 0
    background = 1


class Button(Enum):
    """Buttons on a brick or remote: up, down, left, right, center, left_up, left_down, right_up, right_down, or beacon.

              |          |
    left_up   |    up    | right_up
              |          |
    ---------------------------------
              |          |
    left      |  center  | right
              |          |
    ---------------------------------
              |          |
    left_down |   down   | right_down
              |          |
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
