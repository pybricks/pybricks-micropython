# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from enum import IntEnum


class BatteryType(IntEnum):
    """
    Battery types.

    These values match ``pbdrv_battery_type_t``.
    """

    UNKNOWN = 0
    """
    The battery type is not known.
    """

    ALKALINE = 1
    """
    The batteries are alkaline (e.g. AA/AAA).
    """

    LIION = 2
    """
    The batteries are Li-ion.
    """
