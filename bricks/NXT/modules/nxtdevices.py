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

"""Classes for LEGO MINDSTORMS NXT Devices."""

from _motor import EncodedMotor


class Motor(EncodedMotor):
    """LEGO MINDSTORMS NXT Motor.

    Contained in set:
    8527: LEGO MINDSTORMS NXT (2006)
    9797: LEGO MINDSTORMS Education NXT Base Set (2006)
    8547: LEGO MINDSTORMS NXT 2.0 (2009)
    9842: Separate part (2006)

    LEGO ID: 53787/4297008

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    Pybricks for LEGO MINDSTORMS EV3
    """

    pass


class UltrasonicSensor():
    """LEGO MINDSTORMS NXT Ultrasonic Sensor.

    Contained in set:
    8527: LEGO MINDSTORMS NXT (2006)
    9797: LEGO MINDSTORMS Education NXT Base Set (2006)
    8547: LEGO MINDSTORMS NXT 2.0 (2009)
    9846: Separate part (2006)

    LEGO ID: 53792/4297174

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    """

    pass


class ColorSensor():
    """LEGO MINDSTORMS NXT Color Sensor.

    Contained in set:
    8547: LEGO MINDSTORMS NXT 2.0 (2009)
    9694: Separate part (2009)

    LEGO ID: 64892/6045306

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    """

    pass


class AnalogSensor():
    """Generic class for LEGO MINDSTORMS NXT Analog Sensors. Serves as base class for several LEGO MINDSTORMS NXT Sensors."""

    def __init__(self, port, active=True):
        """Initialize NXT analog sensor.

        Arguments:
            port {const} -- Port to which the device is connected: PORT_1, PORT_2, etc.

        Keyword Arguments:
            active {bool} -- Initialize as active sensor (True) or passive sensor (False). (default: {True})
        """

    def active(self):
        """Set sensor in active mode."""

    def passive(self):
        """Set sensor in passive mode."""

    def value(self):
        """Get the analog sensor value.

        Returns:
            int -- Raw analog value

        """
        return 0

    pass


class LightSensor(AnalogSensor):
    """LEGO MINDSTORMS NXT Light Sensor.

    Contained in set:
    8527: LEGO MINDSTORMS NXT (2006)
    9797: LEGO MINDSTORMS Education NXT Base Set (2006)
    9844: Separate part (2006)

    LEGO ID: 55969/4296917

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    Pybricks for LEGO MINDSTORMS EV3
    """

    pass


class SoundSensor(AnalogSensor):
    """LEGO MINDSTORMS NXT Sound Sensor.

    Contained in set:
    8527: LEGO MINDSTORMS NXT (2006)
    9797: LEGO MINDSTORMS Education NXT Base Set (2006)
    9845: Separate part (2006)

    LEGO ID: 55963/4296969

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    Pybricks for LEGO MINDSTORMS EV3
    """

    pass


class TouchSensor(AnalogSensor):
    """LEGO MINDSTORMS NXT Touch Sensor.

    Contained in set:
    8527: LEGO MINDSTORMS NXT (2006)
    9797: LEGO MINDSTORMS Education NXT Base Set (2006)
    8547: LEGO MINDSTORMS NXT 2.0 (2009)
    9843: Separate part (2006)

    LEGO ID: 53793/4296929

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    Pybricks for LEGO MINDSTORMS EV3
    """

    pass


class TemperatureSensor(AnalogSensor):
    """LEGO MINDSTORMS NXT Temperature Sensor.

    Contained in set:
    9749: Separate part (2009)

    LEGO ID: ?/?

    Compatible with:
    Pybricks for LEGO MINDSTORMS NXT
    Pybricks for LEGO MINDSTORMS EV3
    """

    pass
