"""Robotics module for the Pybricks API."""

from parameters import Stop, Direction


class DriveBase():
    """Class respresenting a robotic vehicle with two powered wheels and optional wheel caster(s)."""

    def __init__(self, left_motor, right_motor, wheel_diameter, axle_track):
        """DriveBase(left_motor, right_motor, wheel_diameter, axle_track)

        Arguments:
            left_motor (Motor): The motor that drives the left wheel.
            right_motor (Motor): The motor that drives the right wheel.
            wheel_diameter (:ref:`dimension`): Diameter of the wheels.
            axle_track (:ref:`dimension`): Distance between the midpoints of the two wheels.

        Example::

            # Initialize two motors and a drive base
            left = Motor(Port.B)
            right = Motor(Port.C)
            robot = DriveBase(left, right, 56, 114)
        """

    def drive(self, speed, steering):
        """Start driving at the specified speed and turnrate, both measured at the center point between the wheels of the robot.

        Arguments:
            speed (:ref:`travelspeed`): Forward speed of the robot.
            steering (:ref:`speed`): Turn rate of the robot.

        Example::

            # Initialize two motors and a drive base
            left = Motor(Port.B)
            right = Motor(Port.C)
            robot = DriveBase(left, right, 56, 114)

            # Initialize a sensor
            sensor = UltrasonicSensor(Port.S4)

            # Drive forward until an object is detected
            robot.drive(100, 0)
            while sensor.distance() > 500:
                wait(10)
            robot.stop()
        """
        pass

    def drive_time(self, speed, steering, time):
        """Drive at the specified speed and turnrate for a given amount of time, and then stop.

        Arguments:
            speed (:ref:`travelspeed`): Forward speed of the robot.
            steering (:ref:`speed`): Turn rate of the robot.
            time (:ref:`time`): Duration of the maneuver.

        Example::

            # Drive forward at 100 mm/s for two seconds
            robot.drive(100, 0, 2000)

            # Turn at 45 deg/s for three seconds
            robot.drive(0, 45, 3000)
        """
        pass

    def stop(self, stop_type=Stop.coast):
        """stop(self, stop_type=Stop.coast)

        Stop the robot.

        Arguments:
            stop_type (Stop): Whether to coast, brake, or hold (*Default*: :class:`Stop.coast <parameters.Stop>`).
        """
        pass
