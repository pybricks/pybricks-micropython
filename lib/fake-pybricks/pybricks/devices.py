"""EV3 motors and sensors."""

from parameters import Stop, Direction


class Motor():
    """LEGO MINDSTORMS EV3 Medium or Large Motor.

    Element 99455/6148292 or 95658/6148278, contained in:

    * 31313: LEGO MINDSTORMS EV3 (2013)
    * 45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    * 45502 or 45502: Separate part (2013)
    """

    def __init__(self, port, direction=Direction.clockwise, gears=None):
        """Motor(port, direction=Direction.clockwise, gears=None)

        Arguments:
            port (Port): Port to which the motor is connected
            direction (Direction): Positive speed direction. (*Default*: Direction.clockwise)
            gears (list): List of gears linked to the motor. (*Default*: ``None``)

                          For example: ``[12, 36]`` represents a gear train with a 12-tooth and a 36-tooth gear. See :ref:`ratio` for illustrated examples.

                          Use a list of lists for multiple gear trains, such as ``[[12, 36], [20, 16, 40]]``.

                          When you specify a gear train, all motor commands and settings are automatically adjusted to account for the resulting gear ratio.

                          For example, with ``gears=[12, 36]``, the gear ratio is 3, which means that the output is mechanically slowed down by a factor of 3. To compensate, the motor will automatically turn 3 times as fast and 3 times as far when you give a motor command. So when you choose ``run_angle(200, 90)``, your mechanism output simply turns at 200 deg/s by 90 degrees.

                          The same holds for the documentation below: When it says "motor angle" or "motor speed", you can read it as "mechanism output angle" and "mechanism output speed", and so on, because the gear ratio is automatically accounted for.

        """
        pass

    def set_torque_settings(self, limit):
        """Configure the maximum torque that can be applied to the motor. (*Default*: 100)

        This sets the maximum :ref:`duty cycle <duty>` that is applied during any subsequent motor command. This reduces the maximum torque output to a percentage of the absolute maximum stall torque. This is useful to avoid applying the full motor torque to a geared or lever mechanism.

        Arguments:
            limit (:ref:`percentage`): Relative torque limit during ``duty`` or ``run`` commands.
        """
        pass

    def set_run_settings(self, max_speed, acceleration, deceleration):
        """Configure the maximum speed, acceleration, and deceleration of the motor for all of the ``run`` commands. See also the :ref:`default parameters <defaultpars>` for each motor.

        Arguments:
            max_speed (:ref:`speed`): Maximum speed of the motor.
            acceleration (:ref:`acceleration`): Acceleration towards the target speed.
            deceleration (:ref:`acceleration`): Deceleration towards standstill.
        """
        pass

    def set_control_settings(self, kp, ki, kd, pid_reset_time, angle_tolerance, speed_tolerance, stall_speed, stall_time):
        """Configure the settings of the position and speed controllers. See also :ref:`pid` and the :ref:`default parameters <defaultpars>` for each motor.

        Arguments:
            kp (int): Proportional position (and integral speed) control constant
            ki (int): Integral position control constant.
            kd (int): Derivative position (and proportional speed) control constant.
            pid_reset_time (:ref:`time`): If you execute any of the ``run`` command within this interval after starting the previous command, the motor will keep its accumulated PID errors. This provides smoother motion when you call any of the ``run`` commands in a tight loop.
            angle_tolerance (:ref:`angle`): Allowed deviation from the target angle before motion is considered complete.
            speed_tolerance (:ref:`speed`): Allowed deviation from zero speed before motion is considered complete.
            stall_speed (:ref:`speed`): If the motor moves slower than ``stall_speed`` for ``stall_time`` during any of the ``run`` commands, the motor is considered to be stalled.
            stall_time (:ref:`time`): See ``stall_speed``.
        """
        pass

    def duty(self, duty):
        """Set the :ref:`duty cycle <duty>` of the motor.

        Arguments:
            duty (percentage): The duty cycle (-100.0 to 100).
        """
        pass

    def angle(self):
        """Get the rotation angle of the motor.

        Returns:
            :ref:`angle`: Motor angle.

        """
        pass

    def speed(self):
        """Get the speed (angular velocity) of the motor.

        Returns:
            :ref:`speed`: Motor speed.

        """
        pass

    def reset_angle(self, angle=0):
        """Reset the accumulated rotation angle of the motor.

        Arguments:
            angle (:ref:`angle`): Value to which the angle should be reset (*Default*: 0).
        """
        pass

    def stop(self, stop_type=Stop.coast):
        """stop(self, stop_type=Stop.coast)

        Stop the motor.

        Arguments:
            stop_type (Stop): Whether to coast, brake, or hold (*Default*: :class:`Stop.coast <parameters.Stop>`).
        """
        pass

    def run(self, speed):
        """Keep the motor runnning at a constant speed (angular velocity).

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. This continues in the background until you give the motor a new command or until the program stops.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
        """
        pass

    def run_time(self, speed, time, stop_type=Stop.coast, wait=True):
        """run_time(self, speed, time, stop_type=Stop.coast, wait=True)

        Run the motor at a constant speed (angular velocity) for a given amount of time.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time to reach standstill after the specified duration.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            time (:ref:`time`): Duration of the maneuver.
            stop_type (Stop): Whether to coast, brake, or hold after coming to standstill (*Default*: :class:`Stop.coast <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits for the specified ``time``.
        """
        pass

    def run_angle(self, speed, rotation_angle, stop_type=Stop.coast, wait=True):
        """run_angle(self, speed, rotation_angle, stop_type=Stop.coast, wait=True)

        Run the motor at a constant speed (angular velocity) by a given angle.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time so that it comes to standstill after traversing the given angle.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            rotation_angle (:ref:`time`): Angle by which the motor should rotate.
            stop_type (Stop): Whether to coast, brake, or hold after coming to standstill (*Default*: :class:`Stop.coast <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits until the motor has traveled precisely the requested angle.
        """
        pass

    def run_target(self, speed, target_angle, stop_type=Stop.coast, wait=True):
        """run_target(self, speed, target_angle, stop_type=Stop.coast, wait=True)

        Run the motor at a constant speed (angular velocity) towards a given target angle.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time so that it comes to standstill at the given target angle.

        The direction of rotation is automatically selected based on the target angle.

        Arguments:
            speed (:ref:`speed`): Absolute speed of the motor. The direction will be automatically selected based on the target angle: it makes no difference if you specify a positive or negative speed.
            target_angle (:ref:`time`): Target angle that the motor should go to, regardless of its current angle.
            stop_type (Stop): Whether to coast, brake, or hold after coming to standstill (*Default*: :class:`Stop.coast <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits until the motor has reached the target angle.
        """
        pass

    def run_until_stalled(self, speed, torque_limit, stop_type=Stop.coast):
        """run_until_stalled(self, speed, torque_limit, stop_type=Stop.coast)

        Run the motor at a constant speed (angular velocity) until it stalls.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, until it encounters an obstacle that prevents it from turning any further. Then the motor stops.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            torque_limit (:ref:`percentage`): Maximum duty cycle used during this command. It works just like :func:`.set_torque_limit`, but it is temporary: once this command completes the motor uses the default or user-specified torque limit again. For example, this allows you to avoid high torques when resetting a mechanism, while the motor operates normally afterwards.
            stop_type (Stop): Whether to coast, brake, or hold after coming to standstill (*Default*: :class:`Stop.coast <parameters.Stop>`).

        Returns:
            :ref:`angle`: Angle at which the motor became stalled.

        """
        pass


class TouchSensor():
    """LEGO MINDSTORMS EV3 Touch Sensor.

    Element 95648/6138404, contained in:

    * 31313: LEGO MINDSTORMS EV3 (2013)
    * 45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    * 45507: Separate part (2013)

    """

    def __init__(self, port):
        """TouchSensor(port)

        Arguments:
            port (Port): Port to which the sensor is connected.
        """
        pass

    def pressed(self):
        """Check if the sensor is pressed.

        Returns:
            :obj:`bool`: ``True`` if the sensor is pressed, ``False`` if it is not pressed.

        """
        pass


class ColorSensor():
    """LEGO MINDSTORMS EV3 Color Sensor.

    Element 95650/6128869, contained in:

    * 31313: LEGO MINDSTORMS EV3 (2013)
    * 45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    * 45506: Separate part (2013)
    """

    def __init__(self, port):
        """ColorSensor(port)

        Arguments:
            port (Port): Port to which the sensor is connected.

        """
        pass

    def color(self):
        """Measure the color of a surface.

        :returns:
            ``Color.black``, ``Color.blue``, ``Color.green``, ``Color.yellow``, ``Color.red``, ``Color.white``, ``Color.brown`` or ``None``.
        :rtype: :class:`Color <parameters.Color>`, or ``None`` if no color is detected.
        """
        pass

    def ambient(self):
        """Measure the ambient light intensity.

        Returns:
            :ref:`percentage`: Ambient light intensity, ranging from 0 (dark) to 100 (bright).
        """
        pass

    def reflection(self):
        """Measure the reflection of a surface to using a red light.

        Returns:
            :ref:`percentage`: Reflection, ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        pass

    def rgb(self):
        """Measure the reflection of a surface using a red, green, and then a blue light.

        Returns:
            tuple of three :ref:`percentages <percentage>`: Reflection for red, green, and blue light, each ranging from 0.0 (no reflection) to 100.0 (high reflection).

        """
        pass


class InfraredSensor():
    """LEGO MINDSTORMS EV3 Infrared Sensor and Beacon.

    Element 95654/6132629 and 72156/6127283, contained in:

    * 31313: LEGO MINDSTORMS EV3 (2013)
    * 45509 and 45508: Separate parts (2013)
    """

    def __init__(self, port):
        """InfraredSensor(port)

        Arguments:
            port (Port): Port to which the sensor is connected.

        """
        pass

    def distance(self):
        """Measure the relative distance between the sensor and an object using infrared light.

        Returns:
            :ref:`relativedistance`: Relative distance ranging from 0 (closest) to 100 (farthest).

        """
        pass

    def beacon(self, channel):
        """Measure the relative distance and angle between the remote and the infrared sensor.

        Arguments:
            channel (int): Channel number of the remote

        :returns: Tuple of relative distance (0 to 100) and approximate angle (-75 to 75 degrees) between remote and infrared sensor.
        :rtype: (:ref:`relativedistance`, :ref:`angle`) or (``None``, ``None``) if no remote is detected.
        """
        pass

    def buttons(self, channel):
        """Check which buttons of the infrared remote are pressed.

        Arguments:
            channel (int): Channel number of the remote

        :returns: List of pressed buttons on the remote on the specified channel.
        :rtype: List of :class:`Button <parameters.Button>`

        """
        pass


class GyroSensor():
    """LEGO MINDSTORMS EV3 Gyro Sensor.

    Element 99380/6138411, contained in:

    * 45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    * 45505: Separate part (2013)
    """

    def __init__(self, port):
        """GyroSensor(port)

        Arguments:
            port (Port): Port to which the sensor is connected.

        """
        pass

    def reset(self):
        """Force sensor to reset as if disconnecting and reconnecting it.

        This can take up to 3 seconds.
        """
        pass

    def rate(self):
        """Measure the angular velocity of the sensor.

        Returns:
            :ref:`speed`: Angular velocity (degrees per second).

        """
        return self.value(1)

    def angle(self):
        """Get the accumulated angle of the sensor.

        Returns:
            :ref:`angle`: Rotation angle (degrees) since initialization or last reset.

        """
        return self.value(0)


class UltrasonicSensor():
    """LEGO MINDSTORMS EV3 Ultrasonic Sensor.

    Element 95652/6138403, contained in:

    * 45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
    * 45504: Separate part (2013)

    """

    def __init__(self, port):
        """UltrasonicSensor(port)

        Arguments:
            port (Port): Port to which the sensor is connected.

        """
        pass

    def distance(self, silent=False):
        """Measure the distance between the sensor and an object using ultrasonic sound waves.

        Arguments:
            silent (bool): Choose ``True`` to turn the sensor off after measuring the distance.

                           Choose ``False`` to leave the sensor on (*Default*).

                           When you choose ``silent=True``, the sensor does not emit sounds waves
                           except when taking the measurement. This reduces interference with
                           other ultrasonic sensors, but turning the sensor off takes approximately 300 ms each time.

        Returns:
            :ref:`distance`: Distance (millimeters).

        """
        pass

    def presence(self):
        """Check for the presence of other ultrasonic sensors by detecting ultrasonic sounds.

        If the other ultrasonic sensor operates in silent mode, you can only detect the presence of that sensor while it is taking a measurement.

        Returns:
            :obj:`bool`: ``True`` if ultrasonic sounds are detected, ``False`` if not.
        """
        pass

    def send(self, number):
        """Transmit an encoded number using ultrasonic waves.

        The number is converted to a binary sequence, which is transmitted by turning the sensor on and off. The message always starts with one pulse as the start of the message, followed by 8 "bits" where the sensor is either on or off during a short interval.

        Arguments:
            number (int): The number to transmit (0-255)
        """

    def receive(self):
        """Wait for a message from another ultrasonic sensor and convert it to a number.

        Returns:
            :obj:`int`: The received number (0-255)
        """
