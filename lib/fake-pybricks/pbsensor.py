"""Sensors are input devices."""

from _pbport import Port


class TouchSensor():
    """Represents a touch sensor with two states, pressed and released"""
    def __init__(self, port: Port):
        """Creates a new instance

        Raises:
            ValueError: The *port* is not a valid port
            RuntimeError: There is not a touch sensor connected to this port
        """
        self._port = port

    @property
    def is_pressed(self) -> bool:
        """Checks if the touch sensor is pressed.

        Returns:
            ``True`` if the sensor is pressed, otherwise ``False``
        """
        return False
