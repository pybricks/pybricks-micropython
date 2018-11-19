"""Experimental features to be rewritten in C code at a later stage."""

from ev3devices import Run, Stop

class Mechanism():
    """Class to control a motor with predefined target angles."""

    def __init__(self, motor, speed, targets, after_stop, reset_forward, reset_torque):
        """Initialize the mechanism settings.

        Arguments:
            motor {motor} -- Previously initialized motor object
            speed {int} -- Mechanism speed while moving to a target
            targets {dict} -- Dictionary of keys (e.g. strings, colors, or sensor values) with corresponding mechanism target angles
            after_stop {const} -- What to do after reaching a target: COAST, BRAKE, or Stop.hold
            reset_forward {bool} -- Go forward until hitting the endstop (True) or go backwards until hitting the endstop (False) (default: {True})
            reset_torque {int} -- Percentage of the maximum torque applied while resetting
        """

        self.motor = motor
        self.speed_abs = speed if speed > 0 else -speed
        self.targets = targets
        self.after_stop = after_stop
        self.reset_forward = reset_forward
        self.reset_torque = reset_torque
        if 'reset' not in self.targets.keys():
            # TODO: Raise error
            pass

    def reset(self):
        """Move towards the endstop and reset angle accordingly."""
        # Get speed with sign
        speed = self.speed_abs if self.reset_forward else -self.speed_abs

        # Temporarily set specified duty limit (TODO: First GET old setting so we can return it afterwards)
        self.motor.settings(self.reset_torque, 0, 500, 5, 1000, 1, 1000, 1000, 100, 800, 800, 5) #( TODO: Implement keyword args to change only the two relevant settings)
        self.motor.run_stalled(speed, Stop.hold, Run.foreground)
        self.motor.reset_angle(self.targets['reset'])
        self.motor.settings(100, 2, 500, 5, 1000, 1, 1000, 1000, 100, 800, 800, 5) #( TODO: Implement keyword args to change only the two relevant settings)

        # Because reset_angle coasts the motor, ensure we stay on reset target with configured stop type
        self.go('reset')

    def go(self, target_key, run_type=Run.foreground):
        """Go to the target specified by the key."""
        # TODO: make speed and after_stop type optional as well, defaulting to initialized values
        self.motor.run_target(self.speed_abs, self.targets[target_key], self.after_stop, run_type)
