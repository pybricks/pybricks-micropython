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


class VirtualBattery:
    """
    A virtual battery driver with sensible defaults.
    """

    voltage: int = 7000
    """
    Provides the battery voltage in mV returned by ``pbdrv_battery_get_voltage_now()``.

    CPython code should write to this attribute to simulate the current battery
    voltage and the PBIO driver will read this attribute.
    """

    current: int = 100
    """
    Provides the battery current in mA returned by ``pbdrv_battery_get_current_now()``.

    CPython code should write to this attribute to simulate the current battery
    current and the PBIO driver will read this attribute.
    """

    temperature: int = 23000
    """
    Provides the battery temperature in m°C returned by ``pbdrv_battery_get_temperature()``.

    CPython code should write to this attribute to simulate the current battery
    temperature and the PBIO driver will read this attribute.
    """

    type: BatteryType = BatteryType.UNKNOWN
    """
    Provides the battery type returned by ``pbdrv_battery_get_type()``.

    CPython code should write to this attribute to simulate the battery type
    and the PBIO driver will read this attribute.

    Generally, this should only be set once during ``__init__`` and not change
    during runtime since real hubs behave this way.
    """
