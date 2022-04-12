# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


class VirtualCounter:
    count: int = 0
    """
    Provides the counter count returned by ``pbdrv_counter_virtual_get_count()``.

    CPython code should write to this attribute to simulate the current counter
    count and the PBIO driver will read this attribute.
    """

    abs_count: int = 0
    """
    Provides the counter absolute count returned by ``pbdrv_counter_virtual_get_abs_count()``.

    CPython code should write to this attribute to simulate the current counter
    absolute count and the PBIO driver will read this attribute.

    To simulate a device that does not support ``abs_count``, override with a
    property like this::

        @property
        def abs_count(self):
            raise PbioError(PbioErrorCode.NOT_SUPPORTED)
    """

    rate: int = 0
    """
    Provides the counter rate in counts per second returned by
    ``pbdrv_counter_virtual_get_rate()``.

    CPython code should write to this attribute to simulate the current counter
    rate and the PBIO driver will read this attribute.
    """
