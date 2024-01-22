from pybricks.pupdevices import Motor
from pybricks.parameters import Stop

# See pybricks-api for detailed docstrings.
class Car:
    def __init__(self, steer_motor, drive_motors, duty_limit=100):
        # Register one or more provided motors, and stop them.
        self.drive_motors = (drive_motors,) if isinstance(drive_motors, Motor) else drive_motors
        for motor in self.drive_motors:
            motor.stop()

        # Find left and right steering endpoints.
        left_end = steer_motor.run_until_stalled(-300, Stop.COAST, duty_limit)
        right_end = steer_motor.run_until_stalled(300, Stop.COAST, duty_limit)

        # Motor is now at the right endpoint. If we set this to be half the
        # total difference between left and right, 0 will be centered.
        self.max_angle = (right_end - left_end) // 2
        steer_motor.reset_angle(self.max_angle)
        steer_motor.track_target(0)

        # On success, register steer motor.
        self.steer_motor = steer_motor

    def drive_power(self, dc):
        # Set each motor to the given power level, or coast if
        # the given power level is low. This is gives a simple
        # rollout behavior which is nice in most RC applications.
        # Below this level, the motors hardly move anyway.
        for motor in self.drive_motors:
            if abs(dc) > 30:
                motor.dc(dc)
            else:
                motor.stop()

    def drive_speed(self, speed):
        # Set each motor to the given speed. This is useful for
        # fine drive control, even at low speeds.
        for motor in self.drive_motors:
            motor.run(speed)

    def steer(self, percentage):
        # Limit user input.
        percentage = max(-100, min(int(percentage), 100))

        # Allow up to maximum angle with some margin, so we don't push all
        # the way to the mechanical stop we found on initializing.
        # Scale this by user percentage.
        target_angle = (self.max_angle - 10) * percentage // 100

        # Track the angle.
        self.steer_motor.track_target(target_angle)
