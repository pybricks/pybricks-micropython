"""Classes for LEGO MINDSTORMS NXT Devices."""

from motor import EncodedMotor


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
