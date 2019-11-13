# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Classes for LEGO MINDSTORMS EV3 Devices."""

# import those ev3devices that are already written in MicroPython-style C code.
from ev3devices_c import (Motor, InfraredSensor, ColorSensor, TouchSensor,
                          UltrasonicSensor, GyroSensor)
