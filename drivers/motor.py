"""Generic classes for motors and pairs of motors. These classes should be translated into MicroPython style C code in modmotor.c."""


class EncoderlessMotor():
    """Class for motors without encoders."""

    def __init__(self, port, reverse=False):
        """Initialize the motor.

        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.

        Keyword Arguments:
            reverse {bool} -- Invert default "forward" and "backward" directions. (default: {False})
        """
        pass

    def settings(self, max_duty=100):
        """Update the motor settings.

        Keyword Arguments:
            max_duty {int} -- Upper bound on the duty cycle percentage. (default: {100})
        """
        pass

    def duty(self, duty):
        """Set motor duty cycle.

        Arguments:
            duty {int} -- Percentage from -100 to 100
        """
        pass

    def brake(self):
        """Stop by setting duty cycle to zero."""
        pass

    def coast(self):
        """Coast motor by setting it to standby mode."""
        pass


class EncodedMotor(EncoderlessMotor):
    """Class for motors with encoders and optional external gear train."""

    def __init__(self, port, reverse=False, gear_ratio=1):
        """Initialize a motor with encoders.

        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.

        Keyword Arguments:
            reverse {bool} -- Invert default "forward" and "backward" directions. (default: {False})
            gear_ratio {float} -- Absolute gear ratio (the slow down factor) of a gear train. (default: {1})
        """
        pass

    def settings(
            self,
            max_duty=100,
            max_speed=1000,
            tolerance=1,
            acceleration_start=1000,
            acceleration_end=1000,
            tight_loop_time=0.25,
            pos_kp=4,
            pos_ki=3,
            pos_kd=0.05):
        """Change the (default) motor settings.

        Keyword Arguments:
            max_duty {int} -- Upper bound on the duty cycle percentage. (default: {100})
            max_speed {float} -- Maximum speed (deg/s) used by all run commands. (default: {1000})
            tolerance {float} -- Allowed deviation (deg) from target before motion is considered complete. (default: {1})
            acceleration_start {float} -- Acceleration when beginning to move. Positive value in deg/s^2. (default: {1000})
            acceleration_end {float} -- Deceleration when stopping. Positive value in deg/s^2 (default: {1000})
            tight_loop_time {float} -- When a run function is called twice in this interval (measured in seconds), assume that the user is doing their own speed control. Then disable the initial acceleration phase. (default: {0.25})
            pos_kp {float} -- Proportional position control constant (default: {4})
            pos_ki {float} -- Integral position control constant (default: {3})
            pos_kd {float} -- Derivative position control constant (default: {0.05})
        """
        pass

    def position(self):
        """Return the angle of the motor/mechanism (degrees).

        Returns:
            float -- Position of the motor/mechanism (degrees).

        """
        pass

    def speed(self):
        """Return the speed of the motor/mechanism (degrees per second).

        Returns:
            float -- Speed of the motor/mechanism (degrees per second).

        """
        pass

    def run(self, speed):
        """Start and keep running the motor/mechanism at the given speed (degrees per second).

        Arguments:
            speed {float} -- Target speed (degrees per second)
        """
        pass

    def stop(self, smooth=True, after_stop=COAST, wait=True):
        """Stop a motor/mechanism.

        Keyword Arguments:
            smooth {bool} -- Smoothly come to standstill just like with the other run commands (True), or stop immediately (False). (default: {True})
            after_stop {const} -- What to do when the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for complete stop (True) or decelerate in the background (False). (default: {True})
        """
        pass

    def run_time(self, speed, duration, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed for a given duration. Then stop.

        Arguments:
            speed {float} -- Target speed (degrees per second)
            duration {float} -- Total duration (seconds)

        Keyword Arguments:
            after_stop {const} -- What to do when the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """
        pass

    def run_stalled(self, speed, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed until it stalls. Then stop.

        Arguments:
            speed {float} -- Target speed (degrees per second)

        Keyword Arguments:
            after_stop {const} -- What to do when the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})

        Returns:
            float -- If wait is True, then return the position (degrees) at the time of stalling

        """
        pass

    def run_angle(self, speed, angle, after_stop=COAST, wait=True):
        """Run a motor at a given speed and stop precisely at given angle relative to starting point.

        Arguments:
            speed {float} -- Absolute target speed (degrees per second). Run direction is automatically determined based on angle.
            target {float} -- Angle that the motor/mechanism should rotate by (degrees).

        Keyword Arguments:
            after_stop {const} -- What to do when the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})

        """
        pass        

    def run_target(self, speed, target, after_stop=COAST, wait=True):
        """Run a motor at a given speed and stop precisely at given position target.

        Arguments:
            speed {float} -- Absolute target speed (degrees per second). Run direction is automatically determined based on target.
            target {float} -- Target position for the motor/mechanism (degrees)

        Keyword Arguments:
            after_stop {const} -- What to do when the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})

        """
        pass

    def track_target(self, target):
        """Position tracking for use in a control loop.

        Arguments:
            target {float} -- Target position for the motor/mechanism (degrees)

        """
        pass


class EncoderlessDriveBase():
    """Drive a vehicle with two parallel wheels or tracks, powered using simple motors without encoders."""

    def __init__(self, left_motor, right_motor):
        """Initialize a drivebase with two motors without encoders.

        Arguments:
            left_motor {EncoderlessMotor} -- Left motor as seen when the robot drives away from you.
            right_motor {EncoderlessMotor} -- Right motor as seen when the robot drives away from you.
        """
        pass

    def drive_duty(self, drive_duty, steer_duty):
        """Start and keep running the motors at the given duty cycle values.

        Arguments:
            drive_duty {int} -- Percentage from -100 to 100. Positive value means driving forward.
            steer_duty {int} -- Percentage from -100 to 100. Positive value gives clockwise rotation.
        """
        pass

    def brake(self):
        """Stop by setting duty cycle to zero."""
        pass

    def coast(self):
        """Coast both motors by setting them to standby mode."""
        pass


class EncodedDriveBase(EncoderlessDriveBase):
    """Drive a vehicle with two parallel wheels or tracks, powered using motors with encoders."""

    def __init__(self, left_motor, right_motor, wheel_diameter, axle_track):
        """Initialize a drivebase with two motors with encoders.

        Arguments:
            left_motor {EncodedMotor} -- Left motor as seen when the robot drives away from you.
            right_motor {EncodedMotor} -- Right motor as seen when the robot drives away from you.
            wheel_diameter {float} -- Diameter of the wheels (or height of the tracks), measured in centimeters.
            axle_track {float} -- Distance between the midpoints of the wheels, measured in centimeters.
        """
        pass

    def drive(self, base_speed, turn_rate):
        """Start and keep driving the robot at the given base_speed and the given turn_rate.

        Arguments:
            base_speed {float} -- Speed of the robot, measured at the midpoint between the wheels (centimeters per second).
            turn_rate {float} -- Turn rate of the robot around the midpoint between the wheels (degrees per second).
        """

    def stop(self, smooth=True, after_stop=COAST, wait=True):
        """Stop the robot.

        Keyword Arguments:
            smooth {bool} -- Smoothly come to standstill just like with the other drive commands (True), or stop immediately (False). (default: {True})
            after_stop {const} -- What to do after the robot stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for complete stop (True) or decelerate in the background (False). (default: {True})
        """
        pass

    def drive_time(self, base_speed, turn_rate, duration, after_stop=COAST, wait=True):
        """Drive the robot at the given base_speed and the given turn_rate for a specified duration. Then stop.

        Arguments:
            base_speed {float} -- Speed of the robot, measured at the midpoint between the wheels (centimeters per second).
            turn_rate {float} -- Turn rate of the robot around the midpoint between the wheels (degrees per second).
            duration {[type]} -- Total duration (seconds).

        Keyword Arguments:
            after_stop {const} -- What to do after the robot stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to complete (True) or drive in the background (False). (default: {True})
        """
        pass

    def drive_distance(self, base_speed, turn_rate, distance, after_stop=COAST, wait=True):
        """Drive the robot at the given base_speed and the given turn_rate until completing a specified distance, and stop there.

        Arguments:
            base_speed {float} -- Speed of the robot, measured at the midpoint between the wheels (centimeters per second).
            turn_rate {float} -- Turn rate of the robot around the midpoint between the wheels (degrees per second).
            distance {float} -- Total distance to drive, measured at the midpoint between the wheels (centimeters).

        Keyword Arguments:
            after_stop {const} -- What to do after the robot stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to complete (True) or drive in the background (False). (default: {True})
        """
        pass

    def drive_angle(self, base_speed, turn_rate, turn_angle, after_stop=COAST, wait=True):
        """Drive the robot at the given base_speed and the given turn_rate until completing a specified turn_angle, and stop there.

        Arguments:
            base_speed {float} -- Speed of the robot, measured at the midpoint between the wheels (centimeters per second).
            turn_rate {float} -- Turn rate of the robot around the midpoint between the wheels (degrees per second).
            turn_angle {float} -- Total angle to turn (degrees).

        Keyword Arguments:
            after_stop {const} -- What to do after the robot stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to complete (True) or drive in the background (False). (default: {True})
        """
        pass

    def turn_radius_to_turn_rate(self, base_speed, turn_radius):
        """Convert a desired turn_radius to the required turn_rate for a given base_speed.

        Arguments:
            base_speed {float} -- Speed of the robot, measured at the midpoint between the wheels (centimeters per second).
            turn_radius {float} -- Desired turn radius (centimeters). Positive value for clockwise motion when driving forward.

        Returns:
            float -- turn_rate (degrees per second) to obtain the desired turn radius (centimeters).

        """
        pass
