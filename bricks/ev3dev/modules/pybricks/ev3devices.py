# MIT License
#
# Copyright (c) 2017 Laurens Valk
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

"""Classes for LEGO MINDSTORMS EV3 Devices."""

# import those ev3devices that are already written in MicroPython-style C code.
from ev3devices_c import Motor
from .parameters import Color, Button
from .tools import StopWatch, wait

# Import ev3dev sysfs sensor base class and modes
from .ev3devio import Ev3devSensor, Ev3devUartSensor


class TouchSensor(Ev3devSensor):
    """LEGO MINDSTORMS EV3 Touch Sensor.

    Contained in set:
    31313: LEGO MINDSTORMS EV3 (2013)
    45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    45507: Separate part (2013)

    LEGO ID: 95648/6138404

    Compatible with:
    Pybricks for LEGO MINDSTORMS EV3
    """

    _ev3dev_driver_name = 'lego-ev3-touch'

    def pressed(self):
        """Check if the sensor is pressed.

        Returns:
            bool -- True if the sensor is pressed, False if it is not pressed.

        """
        return bool(self._value(0))


class ColorSensor(Ev3devSensor):
    """LEGO MINDSTORMS EV3 Color Sensor.

    Contained in set:
    31313: LEGO MINDSTORMS EV3 (2013)
    45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    45506: Separate part (2013)

    LEGO ID: 95650/6128869

    Compatible with:
    Pybricks for LEGO MINDSTORMS EV3
    """

    _ev3dev_driver_name = 'lego-ev3-color'
    _number_of_values = 3


    def color(self):
        """Check the color of a surface.

        Returns:
            int -- Color.black, Color.blue, Color.green, Color.yellow, Color.red, Color.white, or Color.brown. Returns None if no color is detected.

        """
        self._mode('COL-COLOR')
        color = self._value(0)
        return None if color == 0 else color

    def ambient(self):
        """Measure the ambient light intensity.

        Returns:
            int -- Ambient light intensity, ranging from 0 (dark) to 100 (bright).

        """
        self._mode('COL-AMBIENT')
        return self._value(0)

    def reflection(self):
        """Measure the reflection of a surface (using a red light).

        Returns:
            float -- Reflection, ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        self._mode('REF-RAW')
        return round(max(0, min(self._value(0)*-0.2965+193.6, 100)), 1)

    def rgb(self):
        """Measure the reflection of a surface (using a red, green, and blue light, each measured in turn).

        Returns:
            (float, float, float) -- Reflection for red, green, and blue light, each ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        self._mode('RGB-RAW')
        r = round(max(0, min(self._value(0)*0.258-0.3, 100)), 1)
        g = round(max(0, min(self._value(1)*0.280-0.8, 100)), 1)
        b = round(max(0, min(self._value(2)*0.523-3.7, 100)), 1)
        return r, g, b


class InfraredSensor(Ev3devSensor):
    """LEGO MINDSTORMS EV3 Infrared Sensor and Beacon.

    Contained in set:
    31313: LEGO MINDSTORMS EV3 (2013)
    45509 and 45508: Separate parts (2013)

    LEGO ID: 95654/6132629 and 72156/6127283

    Compatible with:
    Pybricks for LEGO MINDSTORMS EV3
    """

    _ev3dev_driver_name = 'lego-ev3-ir'
    _number_of_values = 8
    _combinations = {
        0: [],
        1: [Button.left_up],
        2: [Button.left_down],
        3: [Button.right_up],
        4: [Button.right_down],
        5: [Button.left_up, Button.right_up],
        6: [Button.left_up, Button.right_down],
        7: [Button.left_down, Button.right_up],
        8: [Button.left_down, Button.right_down],
        9: [Button.beacon],
        10: [Button.left_up, Button.left_down],
        11: [Button.right_up, Button.right_down]
    }

    def distance(self):
        """Measure approximate distance based on the intensity of reflected infrared light.

        Returns:
            int -- Relative distance ranging from 0 (closest) to 100 (farthest)

        """
        self._mode('IR-PROX')
        return self._value(0)

    def beacon(self, channel):
        """Return relative distance and angle between active remote and the infrared sensor.

        Arguments:
            channel {int} -- Channel number on the remote

        Returns:
            relative distance between active remote and infrared sensor: 0 (closest) to 100 (farthest),
            approximate angle between active remote and infrared sensor: -75 to 75 (degrees)

        """
        self._mode('IR-SEEK')
        if channel == 1:
            head = self._value(0)
            dist = self._value(1)
        elif channel == 2:
            head = self._value(2)
            dist = self._value(3)
        elif channel == 3:
            head = self._value(4)
            dist = self._value(5)
        elif channel == 4:
            head = self._value(6)
            dist = self._value(7)
        if dist == -128:
            return None, None
        else:
            return(dist, head*3)

    def buttons(self, channel):
        """Return a list of pressed buttons.

        Arguments:
            channel {int} -- Channel number on the remote

        Returns:
            list -- A list of buttons

        """
        self._mode('IR-REMOTE')
        if channel == 1:
            code = self._value(0)
        elif channel == 2:
            code = self._value(1)
        elif channel == 3:
            code = self._value(2)
        elif channel == 4:
            code = self._value(3)
        return self._combinations[code]


class GyroSensor(Ev3devUartSensor):
    """LEGO MINDSTORMS EV3 Gyro Sensor.

    Contained in set:
    45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    45505: Separate part (2013)

    LEGO ID: 99380/6138411

    Compatible with:
    Pybricks for LEGO MINDSTORMS EV3
    """

    _ev3dev_driver_name = 'lego-ev3-gyro'
    _number_of_values = 2
    _default_mode = 'GYRO-G&A'

    def __init__(self, port):
        Ev3devUartSensor.__init__(self, port)
        self.reset_angle()

    def rate(self):
        return self._value(1)

    def angle(self):
        return self._value(0) - self.offset

    def reset_angle(self, angle=0):
        self.offset = self._value(0) - angle

    def calibrate(self):
        self._reset()
        self.reset_angle(0)


class UltrasonicSensor(Ev3devSensor):
    """LEGO MINDSTORMS EV3 Ultrasonic Sensor.

    Contained in set:
    45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    45504: Separate part (2013)

    LEGO ID: 95652/6138403

    Compatible with:
    Pybricks for LEGO MINDSTORMS EV3
    """

    _ev3dev_driver_name = 'lego-ev3-us'

    PING_WAIT = 300

    def distance(self, turn_off=False):
        """Measure distance to the nearest object using ultrasonic waves.

        Keyword Arguments:
            turn_off {bool} -- Set to True to turn the sensor off after measuring the distance.
                               Doing so reduces interference with other ultrasonic sensors, but
                               it takes approximately 300 ms. (default: {False})

        Returns:
            int -- Distance (millimeters).

        """
        if turn_off:
            self.mode_now = None
            self._mode('US-SI-CM')
            return self._value(0)
            wait(self.PING_WAIT)
        else:
            self._mode('US-DIST-CM')
            return self._value(0)

    def presence(self):
        """Look for the presence of other ultrasonic sensors by checking for ultrasonic sounds.

        Returns:
            bool -- True if ultrasonic sounds are detected, False if not.

        """
        self._mode('US-LISTEN')
        return self._value(0)
