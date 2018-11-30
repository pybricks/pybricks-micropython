# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Sensors"""

from uselect import poll

from uev3dev._sysfs import find_node
from uev3dev._sysfs import Attribute
from uev3dev._sysfs import IntAttribute
from uev3dev._sysfs import ListAttribute

_POLLPRI = 0x0002


class SensorNotFoundError(Exception):
    """Exception thrown when a sensor is not found"""
    def __init__(self, name, port):
        msg = name + ' not found on port ' + port
        super(SensorNotFoundError, self).__init__(msg)


class Sensor():
    """Object that represents a sensor."""

    def __init__(self, port, driver):
        """Create a new sensor object.

        :param string port: The port name (``'1'``, ``'2'``, ``'3'`` or
            ``'4'``).
        :param string driver: The kernel driver name.
        """
        # Handle shorthand port names
        if len(port) == 1:
            port = 'ev3-ports:in' + port
        node = find_node('lego-sensor', port, driver)
        if not node:
            raise SensorNotFoundError(self.__class__.__name__, port)

        try:
            self._command = Attribute(node, 'command', 'w')
            self._commands = ListAttribute(node, 'commands', 'r').read()
        except OSError:
            # some sensors do no support commands
            pass

        self._decimals = IntAttribute(node, 'decimals', 'r')
        self._mode = Attribute(node, 'mode', 'r+')
        self._modes = ListAttribute(node, 'modes', 'r').read()
        self._num_values = IntAttribute(node, 'num_values', 'r')
        self._units = Attribute(node, 'units', 'r')
        self._value = (IntAttribute(node, 'value0', 'r'),
                       IntAttribute(node, 'value1', 'r'),
                       IntAttribute(node, 'value2', 'r'),
                       IntAttribute(node, 'value3', 'r'),
                       IntAttribute(node, 'value4', 'r'),
                       IntAttribute(node, 'value5', 'r'),
                       IntAttribute(node, 'value6', 'r'),
                       IntAttribute(node, 'value7', 'r'))
        self._cached_decimals = None
        self._cached_num_values = None
        self._cached_units = None

    @property
    def units(self):
        """Gets the units of measurement for the current mode.

        :returns str: The units.
        """
        if self._cached_units is None:
            self._cached_units = self._units.read()
        return self._cached_units

    @property
    def num_values(self):
        """Gets the number of data values for the current mode.

        :returns str: The number of values.
        """
        if self._cached_num_values is None:
            self._cached_num_values = self._num_values.read()
        return self._cached_num_values

    @property
    def _decimals_(self):
        if self._cached_decimals is None:
            self._cached_decimals = self._decimals.read()
        return self._cached_decimals

    def set_mode(self, mode):
        """Sets the mode of the sensor.

        :param str mode: The name of the mode.
        """
        if mode not in self._modes:
            raise ValueError('Invalid mode: ' + mode)
        self._mode.write(mode)
        self._cached_decimals = None
        self._cached_num_values = None
        self._cached_units = None

    def value(self, index):
        """Gets a sensor data value.

        :param int index: The index of the value (0 to 7).
        :returns: the value read
        """
        if index < 0 or index >= self.num_values:
            raise ValueError('Index is out of range')
        value = self._value[index].read()
        decimals = self._decimals_
        if decimals:
            value /= 10 ** decimals
            value = round(value, decimals)
        return value


class EV3ColorSensor(Sensor):
    """Object that represents a LEGO EV3 Color sensor"""

    _COLORS = (None, 'black', 'blue', 'green', 'yellow', 'red', 'white',
               'brown')

    def __init__(self, port):
        """Create a new instance of a color sensor.

        :param string port: The input port the sensor is connected to (``'1'``,
            ``'2'``, ``'3'`` or ``'4'``).
        """
        super(EV3ColorSensor, self).__init__(port, 'lego-ev3-color')
        self._current_mode = self._modes.index(self._mode.read())

    def read_reflected(self):
        """Reads the current reflected light value of the sensor.

        Also has the effect of setting the LED to red.

        :return int: A percentage value.
        """
        if self._current_mode != self._modes[0]:
            self.set_mode(self._modes[0])
        return self.value(0)

    def read_ambient(self):
        """Reads the current ambient light value of the sensor.

        Also has the effect of setting the LED to dim blue.

        :return int: A percentage value.
        """
        if self._current_mode != self._modes[1]:
            self.set_mode(self._modes[1])
        return self.value(0)

    def read_color(self):
        """Reads the current color value from the sensor.

        Also has the effect of setting the LED to white.

        :return str: One of None, 'black', 'blue', 'green', 'yellow', 'red',
            'white', or 'brown'
        """
        if self._current_mode != self._modes[2]:
            self.set_mode(self._modes[2])
        return self._COLORS[int(self.value(0))]

    def read_raw_rgb(self):
        """Reads the current raw red, green, and blue reflected light values.

        Also has the effect of setting the LED to white.

        :return int, int, int: the red, green, and blue component values.
        """
        if self._current_mode != self._modes[4]:
            self.set_mode(self._modes[4])
        return self.value(0), self.value(1), self.value(2)


class EV3UltrasonicSensor(Sensor):
    """Object that represents the LEGO EV3 Ultrasonic Sensor"""
    def __init__(self, port):
        """Create a new instance of an ultrasonic sensor.

        :param string port: The input port the sensor is connected to.
        """
        super(EV3UltrasonicSensor, self).__init__(port, 'lego-ev3-us')
        self._current_mode = self._modes.index(self._mode.read())

    def read_cm(self):
        """Reads the current distance measured by the sensor in centimeters.

        Also has the effect of setting the LED to red.

        :return float: The distance.
        """
        if self._current_mode != self._modes[0]:
            self.set_mode(self._modes[0])
        return self.value(0)

    def read_inch(self):
        """Reads the current distance measured by the sensor in inches.

        Also has the effect of setting the LED to red.

        :return float: The distance.
        """
        if self._current_mode != self._modes[1]:
            self.set_mode(self._modes[1])
        return self.value(0)

    def listen(self):
        """Reads the current distance measured by the sensor in inches.

        Also has the effect making the LED blink red.

        :return bool: ``True`` if another ultrasonic sensor was detected,
            otherwise ``False``.
        """
        if self._current_mode != self._modes[2]:
            self.set_mode(self._modes[2])
        return bool(self.value(0))


class EV3TouchSensor(Sensor):
    """Object that represents the LEGO EV3 Touch Sensor"""

    RELEASED = 0
    PRESSED = 1
    BUMPED = 2

    def __init__(self, port):
        """Create a new instance of an ultrasonic sensor.

        :param string port: The input port the sensor is connected to.
        """
        super(EV3TouchSensor, self).__init__(port, 'lego-ev3-touch')
        self._current_mode = self._modes.index(self._mode.read())

    def wait(self, state):
        if state == self.RELEASED:
            while self._value[0].read():
                pass
        elif state == self.PRESSED:
            while not self._value[0].read():
                pass
        elif state == self.BUMPED:
            p = poll()
            p.register(self._value[0].attr.fileno(), _POLLPRI)
            if not self._value[0].read():
                p.poll()
            if self._value[0].read():
                p.poll()

        else:
            raise ValueError('Invalid state')
