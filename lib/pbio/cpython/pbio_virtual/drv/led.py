# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from ..error import PbioError, PbioErrorCode


class VirtualLed:
    def on_set_hsv(self, timestamp: int, h: int, s: int, v: int) -> None:
        """
        This method called from ``pbdrv_led_virtual_set_hsv()``.

        The default implementation raises ``PBIO_ERROR_NOT_IMPLEMENTED`` so
        overriding methods should not call ``super().on_event_poll()``.

        Args:
            timestamp: The time when the value was set as 32-bit unsigned microseconds.
            h: The requested hue (0 - 359).
            s: The requested saturation (0 - 100).
            v: The requested value (0 - 100).
        """
        raise PbioError(PbioErrorCode.NOT_IMPLEMENTED)
