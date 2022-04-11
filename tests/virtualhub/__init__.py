# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


from enum import IntEnum, IntFlag


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


class Buttons(IntFlag):
    """
    Hub buttons.

    These values match ``pbio_button_flags_t``.
    """

    LEFT_DOWN = 1 << 1
    DOWN = 1 << 2
    RIGHT_DOWN = 1 << 3
    LEFT = 1 << 4
    CENTER = 1 << 5
    RIGHT = 1 << 6
    LEFT_UP = 1 << 7
    UP = 1 << 8
    RIGHT_UP = 1 << 9


class VirtualHub:
    """
    Base class for virtual hub implementations.
    """

    def on_event_poll(self) -> None:
        """
        This method is called during MICROPY_EVENT_POLL_HOOK in MicroPython.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_event_poll()``.
        """
        pass

    def on_light(self, id: int, r: int, g: int, b: int) -> None:
        """
        This method is from the ``led_virtual`` driver when the color of the
        light is set.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_light()``.

        Args:
            id: The light identifier (0-based index).
            r: The requested red value (0 - 255).
            g: The requested green value (0 - 255).
            b: The requested blue value (0 - 255).
        """
        pass

    @property
    def battery_voltage(self) -> int:
        """
        Gets the current battery voltage.

        The virtual battery driver uses this value.

        The default implementation returns a constant value of 7V.

        Returns:
            The current battery voltage in mV.
        """
        return 7000

    @property
    def battery_current(self) -> int:
        """
        Gets the current battery current.

        The virtual battery driver uses this value.

        The default implementation returns a constant value of 100mA.

        Returns:
            The current current voltage in mA.
        """
        return 100

    @property
    def battery_temperature(self) -> int:
        """
        Gets the current battery temperature.

        The virtual battery driver uses this value.

        The default implementation returns a constant value of 23°C.

        Returns:
            The current current temperature in m°C.
        """
        return 23000

    @property
    def battery_type(self) -> BatteryType:
        """
        Gets the battery type.

        The virtual battery driver uses this value.

        The default implementation returns a constant value of
        ``BatteryType.UNKNOWN``.

        Returns:
            The the battery type.
        """
        return BatteryType.UNKNOWN

    @property
    def buttons(self) -> Buttons:
        """
        Gets the buttons that are currently pressed.

        The virtual button driver uses this value.

        The default implementation returns a constant value of ``Buttons(0)``.

        Returns:
            The button flags of the currently pressed buttons.
        """
        return Buttons(0)

    @property
    def counter_count(self) -> Sequence[int]:
        """
        Gets the current counter count for each counter.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6

    @property
    def counter_abs_count(self) -> Sequence[int]:
        """
        Gets the current counter absolute count for each counter.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6

    @property
    def counter_abs_rate(self) -> Sequence[int]:
        """
        Gets the current counter rate for each counter in counts per second.

        The virtual counter driver uses this value.

        The default implementation returns a constant value of 0 for 6 counters.
        """
        return [0] * 6
