"""EV3 motors and sensors."""

from parameters import Stop, Direction


class Motor():
    """Medium or Large EV3 motor."""

    def __init__(self, port, direction=Direction.clockwise, gears=None):
        """Motor(port, direction=Direction.clockwise, gears=None)

        Arguments:
            port (Port): Port to which the motor is connected
            direction (Direction): Positive speed direction. (*Default*: Direction.clockwise)
            gears (list): List of the number of teeth on each gear in a gear train, as shown in the examples below. Use a list of lists for multiple gear trains. (*Default*: None)
        ::

            Example for one gear train: gears=[12, 36]
             ____________
            |            |
            |    motor   |
            |____________|
                  ||
                  || 12t      36t
                ||||||  ||||||||||||||
                              ||
                              ||
                          output axle


            Example with multiple gear trains: gears=[[12, 20, 36], [20, 40], [20, 8, 40]]
            _____________
            |            |
            |    motor   |
            |____________|
                  ||
                  || 12t    20t           36t
                ||||||  |||||||||||  ||||||||||||||
                                           ||
                                           ||
                                        ||||||||  |||||||||||||||||
                                            20t          || 40t
                                                         ||
                                                      ||||||||  |||  |||||||||||||||||
                                                          20t    8t         || 40t
                                                                            ||
                                                                        output axle
        """
        pass

    def run_time(self, speed):
        """Test docstring."""
        pass
