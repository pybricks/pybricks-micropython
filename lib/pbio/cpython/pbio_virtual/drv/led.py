# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from ..error import PbioError, PbioErrorCode


class VirtualLed:
    def on_set_hsv(self, r: int, g: int, b: int) -> None:
        """
        This method called from ``pbdrv_led_virtual_set_hsv()``.

        The default implementation raises ``PBIO_ERROR_NOT_IMPLEMENTED`` so
        overriding methods should not call ``super().on_event_poll()``.

        Args:
            r: The requested red value (0 - 255).
            g: The requested green value (0 - 255).
            b: The requested blue value (0 - 255).
        """
        raise PbioError(PbioErrorCode.NOT_IMPLEMENTED)
