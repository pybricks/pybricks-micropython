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
from timing import StopWatch, wait

# Import ev3dev sysfs sensor base class and modes
from .ev3devio import Ev3devSensor


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
        return bool(self.value(0))


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

    # TODO: Use Pybricks color enum for values below instead.
    _ev3_colors = {
        0: None,
        1: Color.black,
        2: Color.blue,
        3: Color.green,
        4: Color.yellow,
        5: Color.red,
        6: Color.white,
        7: Color.brown,
    }

    def color(self):
        """Check the color of a surface.

        Returns:
            int -- Color.black, Color.blue, Color.green, Color.yellow, Color.red, Color.white, or Color.brown. Returns None if no color is detected.

        """
        self.mode('COL-COLOR')
        return self._ev3_colors[self.value(0)]

    def ambient(self):
        """Measure the ambient light intensity.

        Returns:
            int -- Ambient light intensity, ranging from 0 (dark) to 100 (bright).

        """
        self.mode('COL-AMBIENT')
        return self.value(0)

    def reflection(self):
        """Measure the reflection of a surface (using a red light).

        Returns:
            float -- Reflection, ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        self.mode('REF-RAW')
        # Todo: verify actual formula
        return round((653-self.value(0))*0.28, 1)

    def rgb(self):
        """Measure the reflection of a surface (using a red, green, and blue light, each measured in turn).

        Returns:
            (float, float, float) -- Reflection for red, green, and blue light, each ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        self.mode('RGB-RAW')
        # TODO: Discuss range: (0-100 vs 0-255 vs 0-1023). Range 0-100 equivalent to reflected mode would be nice.
        return min(self.value(0)*100 >> 9, 100), min(self.value(1)*100 >> 9, 100), min(self.value(2)*100 >> 9, 100)


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
        self.mode('IR-PROX')
        return self.value(0)

    def beacon(self, channel):
        """Return relative distance and angle between active remote and the infrared sensor.

        Arguments:
            channel {int} -- Channel number on the remote

        Returns:
            relative distance between active remote and infrared sensor: 0 (closest) to 100 (farthest),
            approximate angle between active remote and infrared sensor: -75 to 75 (degrees)

        """
        self.mode('IR-SEEK')
        if channel == 1:
            head = self.value(0)
            dist = self.value(1)
        elif channel == 2:
            head = self.value(2)
            dist = self.value(3)
        elif channel == 3:
            head = self.value(4)
            dist = self.value(5)
        elif channel == 4:
            head = self.value(6)
            dist = self.value(7)
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
        self.mode('IR-REMOTE')
        if channel == 1:
            code = self.value(0)
        elif channel == 2:
            code = self.value(1)
        elif channel == 3:
            code = self.value(2)
        elif channel == 4:
            code = self.value(3)
        return self._combinations[code]


class GyroSensor(Ev3devSensor):
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

    def rate(self):
        """Measure the angular velocity of the sensor.

        Returns:
            int -- Angular velocity (degrees per second).

        """
        return self.value(1)

    def angle(self):
        """Get the accumulated angle of the sensor.

        Returns:
            int -- Angle (degrees).

        """
        return self.value(0)

    pass


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

    BITS = 8
    PING_WAIT = 300
    BIT_DURATION = PING_WAIT + 50
    TIME_OUT = 3000
    SUCCESS_COUNT = 50

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
            self.mode('US-SI-CM')
            return self.value(0)
            wait(PING_WAIT)
        else:
            self.mode('US-DIST-CM')
            return self.value(0)

    def presence(self):
        """Look for the presence of other ultrasonic sensors by checking for ultrasonic sounds.

        Returns:
            bool -- True if ultrasonic sounds are detected, False if not.

        """
        self.mode('US-LISTEN')
        return self.value(0)

    # The following two methods are experimental and mainly just for fun. Ultimately,
    # we could have something more generic in the robotics library, for anything that
    # can pulse and sense. So, that could include communication between light sensors
    # or even motors pushing touch sensors.
    def send(self, number):
        """Transmit a number using ultrasonic waves.

        Arguments:
            number {int} -- The number to transmit (0-255)

        """
        # Verify input is in range
        MAX = 2**self.BITS-1
        number = int(number)
        assert 0 <= number <= MAX, "Number must be between 0 and " + str(MAX) + "."

        # Make message as binary list, MSB first
        message = [0 for i in range(self.BITS+1)]
        number += MAX + 1  # Add an extra high bit at start
        for i in range(self.BITS, -1, -1):
            if number % 2:
                message[i] = 1
            number = number // 2

        # Send the message
        watch = StopWatch()
        for i, bit in enumerate(message):
            if bit:
                self.distance(True)
            while watch.time() < (i+1)*self.BIT_DURATION:
                wait(10)

    def receive(self):
        """Wait for an ultrasonic message and convert it to a number.

        Returns:
            int -- The received number (0-255)

        """
        # Wait for bit that signifies start of message
        watch = StopWatch()
        self.mode('US-LISTEN')
        counts = 0
        while watch.time() < self.TIME_OUT:
            if self.value(0):
                counts += 1
            if counts >= self.SUCCESS_COUNT:
                break
        if counts < self.SUCCESS_COUNT:
            raise OSError("Did not receive message.")

        # Reset stopwatch and record time of subsequent bits
        watch.reset()
        watch.resume()
        detections = [0 for bit in range(self.BITS)]
        while watch.time() < (self.BITS+1)*self.BIT_DURATION:
            # If pulse is detected, round to nearest bit and increase detection count for that bit
            if self.value(0):
                bit = round(watch.time()/self.BIT_DURATION)-1
                if 0 <= bit <= self.BITS-1:
                    detections[bit] = detections[bit] + 1

        # Return decode message as decimal number
        return sum(
            [2**(self.BITS-bit-1) if count > self.SUCCESS_COUNT else 0 for bit, count in enumerate(detections)]
        )
