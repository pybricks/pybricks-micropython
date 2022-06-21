# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


class VirtualCounter:
    rotations: int = 0
    """
    Provides the rotations returned by ``pbdrv_counter_virtual_get_angle()``.

    CPython code should write to this attribute to simulate the current value
    and the PBIO driver will read this attribute.
    """

    millidegrees: int = 0
    """
    Provides the millidegrees returned by ``pbdrv_counter_virtual_get_angle()``.

    CPython code should write to this attribute to simulate the current value
    and the PBIO driver will read this attribute.
    """

    millidegrees_abs: int = 0
    """
    Provides the absolute angle returned by ``pbdrv_counter_virtual_get_abs_angle()``.

    CPython code should write to this attribute to simulate the current value
    and the PBIO driver will read this attribute.

    To simulate a device that does not support this, override with a
    property like this::

        @property
        def millidegrees_abs(self):
            raise PbioError(PbioErrorCode.NOT_SUPPORTED)
    """
