# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from ..drv.button import VirtualButtons
from ..drv.battery import VirtualBattery
from ..drv.clock import VirtualClock
from ..drv.counter import VirtualCounter
from ..drv.ioport import VirtualIOPort, PortId
from ..drv.led import VirtualLed
from ..drv.motor_driver import VirtualMotorDriver
from . import DefaultPlatform


class Platform(DefaultPlatform):
    def __init__(self):
        super().__init__()

        self.battery[-1] = VirtualBattery()
        self.button[-1] = VirtualButtons()
        self.clock[-1] = VirtualClock()
        for i in range(6):
            self.counter[i] = VirtualCounter()
        for p in range(PortId.A, PortId.F + 1):
            self.ioport[p] = VirtualIOPort(p)
        self.led[0] = VirtualLed()
        for i in range(6):
            self.motor_driver[i] = VirtualMotorDriver()
