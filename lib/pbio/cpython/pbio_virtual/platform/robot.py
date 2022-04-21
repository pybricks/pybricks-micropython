# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from ..drv.button import VirtualButtons
from ..drv.battery import VirtualBattery
from ..drv.led import VirtualLed
from ..drv.clock import CountingClock
from ..drv.ioport import VirtualIOPort, PortId, IODeviceTypeId

import numpy
import random
import math

from ..physics.motors import SimpleMotor as SimMotor


class VirtualMotorDriver:
    """
    Virtual motor driver chip implementation, with optionally a (simulated)
    dc motor attached to its output wires.
    """

    def __init__(self, sim_motor):
        self.sim_motor = sim_motor
        self.coasting = True

    def on_coast(self, *args):
        """
        Called when ``pbdrv_motor_driver_coast()`` is called.
        """
        # Don't simulate repeated coasts. This also ensures we skip
        # simulating motors that are attached but otherwise not used.
        if self.coasting:
            return

        # Simulate the motor up to the current time, with previous actuation.
        time = args[0] / 1000000
        self.sim_motor.simulate(time)

        # Set new actuation signal from now on.
        self.sim_motor.actuate(time, numpy.array([self.sim_motor.COAST_DUTY]))
        self.coasting = True

    def on_set_duty_cycle(self, *args):
        """
        Called when ``pbdrv_motor_driver_set_duty_cycle()`` is called.
        """

        # Simulate the motor up to the current time, with previous actuation.
        time = args[0] / 1000000
        self.sim_motor.simulate(time)

        # Set new actuation signal from now on.
        self.sim_motor.actuate(time, numpy.array([args[1]]))
        self.coasting = False


class VirtualCounter:
    """
    Virtual counter driver implementation, with optionally a (simulated)
    dc motor with rotation sensors attached to it.
    """

    def get_counter_data(self):
        """Get all encoder data"""

        # Return zeros if there is no motor.
        if self.sim_motor is None:
            return 0, 0, 0

        # Get data from model.
        time = self.clock.microseconds / 1e6
        angle, speed = self.sim_motor.get_output_at_time(time, tolerance=2e-3)

        # Get "absolute" angle.
        mod_angle = angle % 360
        abs_angle = mod_angle if mod_angle < 180 else mod_angle - 360

        # Return all values.
        return angle - self.initial_angle, abs_angle, speed

    def __init__(self, sim_motor, clock):
        # Store references to motor and clock
        self.sim_motor = sim_motor
        self.clock = clock

        # Initialize starting position
        self.initial_angle = 0
        self.initial_angle, _, _ = self.get_counter_data()

    @property
    def count(self):
        """
        Provides the value for ``pbdrv_counter_virtual_get_count()``.
        """
        return self.get_counter_data()[0]

    @property
    def abs_count(self):
        """
        Provides the value for ``pbdrv_counter_virtual_get_abs_count()``.
        """
        return self.get_counter_data()[1]

    @property
    def rate(self):
        """
        Provides the value for ``pbdrv_counter_virtual_get_rate()``.
        """
        return self.get_counter_data()[2]


class Platform:

    # Ports and attached devices.
    PORTS = {
        PortId.A: IODeviceTypeId.TECHNIC_L_ANGULAR_MOTOR,
        PortId.B: IODeviceTypeId.TECHNIC_M_ANGULAR_MOTOR,
        PortId.C: IODeviceTypeId.TECHNIC_M_ANGULAR_MOTOR,
        PortId.D: IODeviceTypeId.TECHNIC_M_ANGULAR_MOTOR,
        PortId.E: IODeviceTypeId.NONE,
        PortId.F: IODeviceTypeId.NONE,
    }

    def on_poll(self, *args):
        # Push clock forward by one tick on each poll.
        self.clock[-1].tick()

    def __init__(self):

        # Initialize devices internal to the hub.
        self.battery = {-1: VirtualBattery()}
        self.button = {-1: VirtualButtons()}
        self.clock = {-1: CountingClock(start=0, fuzz=0)}
        self.led = {0: VirtualLed()}

        # Initialize all ports
        self.ioport = {}
        self.counter = {}
        self.motor_driver = {}
        self.sim_motor = {}

        for i, (port_id, type_id) in enumerate(self.PORTS.items()):
            # Initialize IO Port.
            self.ioport[port_id] = VirtualIOPort(port_id)
            self.ioport[port_id].motor_type_id = type_id

            # Initialize motor simulation model.
            self.sim_motor[i] = None
            if type_id != IODeviceTypeId.NONE:

                # Get current time.
                initial_time = self.clock[-1].microseconds / 1000000

                # Random initial motor angle with zero speed.
                initial_angle = random.randint(-180, 179)
                initial_speed = 0
                initial_state = numpy.array(
                    [
                        math.radians(initial_angle),
                        math.radians(initial_speed),
                    ]
                )

                # Initialize simulated motor.
                self.sim_motor[i] = SimMotor(t0=initial_time, x0=initial_state)

            # Initialize counter and motor drivers with the given motor.
            self.counter[i] = VirtualCounter(self.sim_motor[i], self.clock[-1])
            self.motor_driver[i] = VirtualMotorDriver(self.sim_motor[i])
