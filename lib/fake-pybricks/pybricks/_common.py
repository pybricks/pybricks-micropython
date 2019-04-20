"""Generic cross-platform module for typical hub devices like displays, speakers, and batteries."""

from parameters import Align, Direction, Stop


class Motor():
    """Generic class for a motor with rotational encoders."""

    def __init__(self, port, direction=Direction.CLOCKWISE, gears=None):
        """Motor(port, direction=Direction.CLOCKWISE, gears=None)

        Arguments:
            port (Port): Port to which the motor is connected.
            direction (Direction): Positive speed direction (*Default*: Direction.CLOCKWISE).
            gears (list): List of gears linked to the motor (*Default*: ``None``).

                          For example: ``[12, 36]`` represents a gear train with a 12-tooth and a 36-tooth gear. See :ref:`ratio` for illustrated examples.

                          Use a list of lists for multiple gear trains, such as ``[[12, 36], [20, 16, 40]]``.

                          When you specify a gear train, all motor commands and settings are automatically adjusted to account for the resulting gear ratio. The motor direction remains unchanged, no matter how many gears you choose.

                          For example, with ``gears=[12, 36]``, the gear ratio is 3, which means that the output is mechanically slowed down by a factor of 3. To compensate, the motor will automatically turn 3 times as fast and 3 times as far when you give a motor command. So when you choose ``run_angle(200, 90)``, your mechanism output simply turns at 200 deg/s for 90 degrees.

                          The same holds for the documentation below: When it states "motor angle" or "motor speed", you can read this as "mechanism output angle" and "mechanism output speed", and so on, as the gear ratio is automatically accounted for.

                          The ``gears`` setting is only available for motors with rotation sensors.

        Example::

            # Initialize a motor (by default this means clockwise, without any gears).
            example_motor = Motor(Port.A)

            # Initialize a motor where positive speed values should go counterclockwise
            right_motor = Motor(Port.B, Direction.COUNTERCLOCKWISE)

            # Initialize a motor with a gear train
            robot_arm = Motor(Port.C, Direction.CLOCKWISE, [12, 36])
        """
        pass

    def dc(self, duty):
        """Set the :ref:`duty cycle <duty>` of the motor.

        Arguments:
            duty (:ref:`percentage`): The duty cycle (-100.0 to 100).

        Example::

            # Set the motor duty cycle to 75%.
            example_motor.duty(75)

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

    def stalled(self):
        """Check whether the motor is currently stalled.

        A motor is stalled when it cannot move even with the maximum torque. For example, when something is blocking the motor or your mechanism simply cannot turn any further.

        Specifically, the motor is stalled when the duty cycle computed by the PID controllers has reached the maximum (so ``duty`` = ``duty_limit``) and still the motor cannot reach a minimal speed (so ``speed`` < ``stall_speed``) for a period of at least ``stall_time``.

        You can change the ``duty_limit``, ``stall_speed``, and ``stall_time`` settings using :meth:`.set_dc_settings` and :meth:`.set_pid_settings` in order to change the sensitivity to being stalled.

        Returns:
            bool: ``True`` if the motor is stalled, ``False`` if it is not.

        """
        pass

    def reset_angle(self, angle):
        """Reset the accumulated rotation angle of the motor.

        Arguments:
            angle (:ref:`angle`): Value to which the angle should be reset.
        """
        pass

    def stop(self, stop_type=Stop.COAST):
        """stop(stop_type=Stop.COAST)

        Stop the motor.

        Arguments:
            stop_type (Stop): Whether to coast, brake, or hold (*Default*: :class:`Stop.COAST <parameters.Stop>`).
        """
        pass

    def run(self, speed):
        """Keep the motor running at a constant speed (angular velocity).

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. This continues in the background until you give the motor a new command or the program stops.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
        """
        pass

    def run_time(self, speed, time, stop_type=Stop.COAST, wait=True):
        """run_time(speed, time, stop_type=Stop.COAST, wait=True)

        Run the motor at a constant speed (angular velocity) for a given amount of time.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time to reach standstill after the specified duration.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            time (:ref:`time`): Duration of the maneuver.
            stop_type (Stop): Whether to coast, brake, or hold after coming to a standstill (*Default*: :class:`Stop.COAST <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits for the specified ``time``.
        """
        pass

    def run_angle(self, speed, rotation_angle, stop_type=Stop.COAST, wait=True):
        """run_angle(speed, rotation_angle, stop_type=Stop.COAST, wait=True)

        Run the motor at a constant speed (angular velocity) by a given angle.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time so that it comes to a standstill after traversing the given angle.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            rotation_angle (:ref:`angle`): Angle by which the motor should rotate.
            stop_type (Stop): Whether to coast, brake, or hold after coming to a standstill (*Default*: :class:`Stop.COAST <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits until the motor has traveled precisely the requested angle.
        """
        pass

    def run_target(self, speed, target_angle, stop_type=Stop.COAST, wait=True):
        """run_target(speed, target_angle, stop_type=Stop.COAST, wait=True)

        Run the motor at a constant speed (angular velocity) towards a given target angle.

        The motor will accelerate towards the requested speed and the duty cycle is automatically adjusted to keep the speed constant, even under some load. It begins to decelerate just in time so that it comes to a standstill at the given target angle.

        The direction of rotation is automatically selected based on the target angle.

        Arguments:
            speed (:ref:`speed`): Absolute speed of the motor. The direction will be automatically selected based on the target angle: it makes no difference if you specify a positive or negative speed.
            target_angle (:ref:`angle`): Target angle that the motor should rotate to, regardless of its current angle.
            stop_type (Stop): Whether to coast, brake, or hold after coming to a standstill (*Default*: :class:`Stop.COAST <parameters.Stop>`).
            wait (bool): Wait for the maneuver to complete before continuing with the rest of the program (*Default*: ``True``). This means that your program waits until the motor has reached the target angle.
        """
        pass

    def run_until_stalled(self, speed, stop_type=Stop.COAST, duty_limit=None):
        """run_until_stalled(speed, stop_type=Stop.COAST, duty_limit=default)

        Run the motor at a constant speed (angular velocity) until it stalls. The motor is considered stalled when it cannot move even with the maximum torque. See :meth:`.stalled` for a more precise definition.

        The ``duty_limit`` argument lets you temporarily limit the motor torque during this maneuver. This is useful to avoid applying the full motor torque to a geared or lever mechanism.

        Arguments:
            speed (:ref:`speed`): Speed of the motor.
            stop_type (Stop): Whether to coast, brake, or hold after coming to a standstill (*Default*: :class:`Stop.COAST <parameters.Stop>`).
            duty_limit (:ref:`percentage`): Relative torque limit. This limit works just like :meth:`.set_dc_settings`, but in this case the limit is temporary: it returns to its previous value after completing this command.
        """
        pass

    def track_target(self, target_angle):
        """Track a target angle that varies in time.

        This function is quite similar to :meth:`.run_target`, but speed and acceleration settings are ignored: it will move to the target angle as fast as possible. Instead, you adjust speed and acceleration by choosing how fast or slow you vary the ``target_angle``.

        This method is useful in fast loops where the motor target changes continuously.

        Arguments:
            target_angle (:ref:`angle`): Target angle that the motor should rotate to.

        Example::

            # Initialize motor and timer
            from math import sin
            motor = Motor(Port.A)
            watch = StopWatch()
            amplitude = 90

            # In a fast loop, compute a reference angle
            # and make the motor track it.
            while True:
                # Get the time in seconds
                seconds = watch.time()/1000
                # Compute a reference angle. This produces
                # a sine wave that makes the motor move
                # smoothly between -90 and +90 degrees.
                angle_now = sin(seconds)*amplitude
                # Make the motor track the given angle
                motor.track_target(angle_now)

        """
        pass

    def set_dc_settings(self, duty_limit, duty_offset):
        """Configure the settings to adjust the behavior of the :meth:`.dc` command. This also affects all of the ``run`` commands, which use the :meth:`.dc` method in the background.

        Arguments:
            duty_limit (:ref:`percentage`): Relative torque limit during subsequent motor commands. This sets the maximum duty cycle that is applied during any subsequent motor command. This reduces the maximum torque output to a percentage of the absolute maximum stall torque. This is useful to avoid applying the full motor torque to a geared or lever mechanism, or to prevent your LEGO® train from unintentionally going at full speed. (*Default*: 100).
            duty_offset (:ref:`percentage`): Minimum duty cycle given when you use :meth:`.dc`. This adds a small feed forward torque so that your motor will move even for very low duty cycle values, which can be useful when you create your own feedback controllers (*Default*: 0).
        """
        pass

    def set_run_settings(self, max_speed, acceleration):
        """Configure the maximum speed and acceleration/deceleration of the motor for all run commands.

        This applies to the ``run``, ``run_time``, ``run_angle``, ``run_target``, or ``run_until_stalled`` commands you give the motor. See also the :ref:`default parameters <defaultpars>` for each motor.

        Arguments:
            max_speed (:ref:`speed`): Maximum speed of the motor during a motor command.
            acceleration (:ref:`acceleration`): Acceleration towards the target speed and deceleration towards standstill. This should be a positive value. The motor will automatically change the sign to decelerate as needed.

        Example::

            # Set the maximum speed to 200 deg/s and acceleration to 400 deg/s/s.
            example_motor.set_run_settings(200, 400)

            # Make the motor run for 5 seconds. Even though the speed argument is 300
            # deg/s in this example, the motor will move at only 200 deg/s because of
            # the settings above.
            example_motor.run_time(300, 5000)
        """
        pass

    def set_pid_settings(self, kp, ki, kd, tight_loop_limit, angle_tolerance, speed_tolerance, stall_speed, stall_time):
        """Configure the settings of the position and speed controllers. See also :ref:`pid` and the :ref:`default parameters <defaultpars>` for each motor.

        Arguments:
            kp (int): Proportional position (and integral speed) control constant.
            ki (int): Integral position control constant.
            kd (int): Derivative position (and proportional speed) control constant.
            tight_loop_limit (:ref:`time`): If you execute any of the ``run`` commands within this interval after starting the previous command, the controllers assume that you want to control the speed directly. This means that it will ignore the acceleration setting and immediately begin tracking the speed you give in the ``run`` command. This is useful in a fast loop, where you usually want the motors to respond quickly rather than accelerate smoothly, for example with a line-following robot.
            angle_tolerance (:ref:`angle`): Allowed deviation from the target angle before motion is considered complete.
            speed_tolerance (:ref:`speed`): Allowed deviation from zero speed before motion is considered complete.
            stall_speed (:ref:`speed`): See :meth:`.stalled`.
            stall_time (:ref:`time`): See :meth:`.stalled`.
        """
        pass


class Display():
    """Show images or text on a display."""

    def clear(self):
        """Clear everything on the display."""
        pass

    def text(self, text, coordinate=None):
        """Display text.

        Parameters:
            text (str): The text to display.
            coordinate (tuple): ``(x, y)`` coordinate tuple. It is the top-left corner of the first character. If no coordinate is specified, it is printed on the next line.

        Example::

            # Clear the display
            brick.display.clear()

            # Print ``Hello`` near the middle of the screen
            brick.display.text("Hello", (60, 50))

            # Print ``World`` directly underneath it
            brick.display.text("World")

        """
        pass

    def image(self, file_name, alignment=Align.CENTER, coordinate=None, clear=True):
        """image(file_name, alignment=Align.CENTER, coordinate=None, clear=True)

        Show an image file.

        You can specify its placement either using ``alignment`` or by specifying a ``coordinate``, but not both.

        Arguments:
            file_name (str): Path to the image file. Paths may be absolute or relative from the project folder.
            alignment (Align): Where to place the image (*Default*: Align.CENTER).
            coordinate (tuple): ``(x, y)`` coordinate tuple. It is the top-left corner of the image (*Default*: None).
            clear (bool): Whether to clear the screen before showing the image (*Default*: ``True``).

        Example::

            # Show a built-in image of two eyes looking upward
            brick.display.image(ImageFile.UP)

            # Display a custom image from your project folder
            brick.display.image('pybricks.png')

            # Display a custom image at the top right of the screen, without clearing
            # the screen first
            brick.display.image('arrow.png', Align.TOP_RIGHT, clear=False)
        """
        pass


class Speaker():
    """Play beeps and sound files using a speaker."""

    def beep(self, frequency=500, duration=100, volume=30):
        """Play a beep/tone.

        Arguments:
            frequency (:ref:`frequency`): Frequency of the beep (*Default*: 500).
            duration (:ref:`time`): Duration of the beep (*Default*: 100).
            volume (:ref:`percentage`): Volume of the beep (*Default*: 30).

        Example::

            # A simple beep
            brick.sound.beep()

            # A high pitch (1500 Hz) for one second (1000 ms) at 50% volume
            brick.sound.beep(1500, 1000, 50)
        """
        pass

    def beeps(self, number):
        """Play a number of default beeps with a brief pause in between.

        Arguments:
            number (int): Number of beeps.

        Example::

            # Make 5 simple beeps
            brick.sound.beeps(5)
        """
        pass

    def file(self, file_name, volume=100):
        """Play a sound file.

        Arguments:
            file_name (str): Path to the sound file, including extension.
            volume (:ref:`percentage`): Volume of the sound (*Default*: 100).

        Example::

            # Play one of the built-in sounds
            brick.sound.file(SoundFile.HELLO)

            # Play a sound file from your project folder
            brick.sound.file('mysound.wav')

        """

        pass


class Battery():
    """Get the status of a battery."""

    def voltage(self):
        """Get the voltage of the battery.

        Returns:
            :ref:`voltage`: Battery voltage.
        """
        pass

    def current(self):
        """Get the current supplied by the battery.

        Returns:
            :ref:`current`: Battery current.

        """
        pass
