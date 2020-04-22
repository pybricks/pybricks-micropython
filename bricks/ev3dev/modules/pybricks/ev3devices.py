# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 The Pybricks Authors

"""Classes for LEGO MINDSTORMS EV3 Devices."""

# import those ev3devices that are already written in MicroPython-style C code.
from ev3devices_c import (InfraredSensor, ColorSensor, TouchSensor,
                          UltrasonicSensor, GyroSensor)
from ev3devices_c import Motor as CompatMotor
from pybricks.parameters import Stop


class Motor(CompatMotor):
    """Motor class backwards-compatible with 1.0 EV3 release."""

    def stop(self, stop_type=Stop.COAST):
        """stop method backwards-compatible with 1.0 EV3 release."""
        if stop_type == Stop.COAST:
            super().stop()
        elif stop_type == Stop.BRAKE:
            self.brake()
        elif stop_type == Stop.HOLD:
            self.hold()
        else:
            raise ValueError("Invalid Argument")

    def stalled(self):
        """stalled backwards-compatible with 1.0 EV3 release."""
        return self.control.stalled()

    def set_dc_settings(self, duty_limit, duty_offset):
        """set_dc_settings backwards-compatible with 1.0 EV3 release."""
        self.control.limits(actuation=duty_limit)
        self.control.pid(feed_forward=duty_offset)

    def set_run_settings(self, max_speed, acceleration):
        """set_run_settings backwards-compatible with 1.0 EV3 release."""
        self.control.limits(max_speed, acceleration)

    def set_pid_settings(self, kp, ki, kd, tight_loop_limit, angle_tolerance,
                         speed_tolerance, stall_speed, stall_time):
        """set_pid_settings backwards-compatible with 1.0 EV3 release."""
        self.control.pid(kp, ki, kd)
        self.control.target_tolerances(speed_tolerance, angle_tolerance)
        self.control.stall_tolerances(stall_speed, stall_time)
