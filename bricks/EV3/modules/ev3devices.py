"""Classes for LEGO MINDSTORMS EV3 Devices."""

# import those ev3devices that are already written in MicroPython-style C code.
from ev3devices_c import *

# Import ev3dev sysfs sensor base class and modes
from ev3devio import Ev3devSensor


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
            bool -- True if sensor is pressed and False otherwise.

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
        """Return color id."""
        self.mode('COL-COLOR')
        return self._ev3_colors[self.value(0)]

    def ambient(self):
        """Return ambient light percentage, ranging from 0 (dark) to 100 (bright)."""
        self.mode('COL-AMBIENT')
        return self.value(0)

    def reflected(self):
        """Return reflected light percentage, ranging from 0.0 (no reflection) to 100 (high reflection)."""
        self.mode('REF-RAW')
        # Todo: verify actual formula
        return round((653-self.value(0))*0.28, 1)

    def rgb(self):
        """Return tuple of reflected light intensities for red (0-100), green (0-100) and blue (0-100)."""
        self.mode('RGB-RAW')
        # TODO: Discuss range: (0-100 vs 0-255 vs 0-1023). Range 0-100 equivalent to reflected mode would be nice.
        return min(self.value(0)*100 >> 9, 100), min(self.value(1)*100 >> 9, 100), min(self.value(2)*100 >> 9, 100)


# TODO: replace with pbio enum consistent with other Pybricks devices. (enum number/order does not matter.)
class Button():
    beacon = 1
    left_up = 2
    left_down = 3
    right_up = 4
    right_down = 5


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
        """Return relative distance ranging from 0 (closest) to 100 (farthest)."""
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

    pass
