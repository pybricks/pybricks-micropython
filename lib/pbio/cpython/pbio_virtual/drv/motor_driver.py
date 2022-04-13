# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from ..error import PbioError, PbioErrorCode


class VirtualMotorDriver:
    """
    Virtual motor driver chip implementation.
    """

    def on_coast(self, timestamp: int) -> None:
        """
        Called when ``pbdrv_motor_driver_coast()`` is called.

        The default implementation raises ``PBIO_ERROR_NOT_IMPLEMENTED`` so
        overriding methods should not call ``super().on_coast()``.

        Args:
            timestamp: The time when the value was set as 32-bit unsigned microseconds.
        """
        raise PbioError(PbioErrorCode.NOT_IMPLEMENTED)

    def on_set_duty_cycle(self, timestamp: int, duty_cycle: float) -> None:
        """
        Called when ``pbdrv_motor_driver_set_duty_cycle()`` is called.

        The default implementation raises ``PBIO_ERROR_NOT_IMPLEMENTED`` so
        overriding methods should not call ``super().on_set_duty_cycle()``.

        Args:
            timestamp: The time when the value was set as 32-bit unsigned microseconds.
            duty_cycle: The requested duty cycle (-1.0 to 1.0).
        """
        raise PbioError(PbioErrorCode.NOT_IMPLEMENTED)
