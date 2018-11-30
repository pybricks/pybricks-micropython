# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
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

"""Motors"""

from errno import EINTR

from uselect import poll

from uev3dev._sysfs import Attribute
from uev3dev._sysfs import IntAttribute
from uev3dev._sysfs import ListAttribute
from uev3dev._sysfs import find_node

# FIXME: uselect does not have POLLPRI
_POLLPRI = 0x0002


class MotorNotFoundError(Exception):
    """Exception thrown when a motor is not found"""
    def __init__(self, name, port):
        msg = name + ' not found on port ' + port
        super(MotorNotFoundError, self).__init__(msg)


class Motor():
    """Object that represents a motor with position feedback.

    Use :py:class:`LargeMotor` or :py:class:`MediumMotor` to create a new
    motor instance rather than using this class directly.
    """

    def __init__(self, port, driver):
        if len(port) == 1:
            port = 'ev3-ports:out' + port
        node = find_node('tacho-motor', port, driver)
        if not node:
            raise MotorNotFoundError(self.__class__.__name__, port)
        self._command = Attribute(node, 'command', 'w')
        self._commands = ListAttribute(node, 'commands', 'r').read()
        self._count_per_rot = IntAttribute(node, 'count_per_rot', 'r').read()
        self._driver_name = Attribute(node, 'driver_name', 'r').read()
        self._duty_cycle = IntAttribute(node, 'duty_cycle', 'r')
        self._duty_cycle_sp = IntAttribute(node, 'duty_cycle_sp', 'r+')
        self._max_speed = IntAttribute(node, 'max_speed', 'r').read()
        self._position = IntAttribute(node, 'position', 'r')
        self._position_sp = IntAttribute(node, 'position_sp', 'r+')
        self._ramp_up_sp = IntAttribute(node, 'ramp_up_sp', 'r+')
        self._ramp_down_sp = IntAttribute(node, 'ramp_down_sp', 'r+')
        self._speed_sp = IntAttribute(node, 'speed_sp', 'r+')
        self._state = ListAttribute(node, 'state', 'r')
        self._stop_action = Attribute(node, 'stop_action', 'r+')
        self._stop_actions = ListAttribute(node, 'stop_actions', 'r').read()
        self._time_sp = IntAttribute(node, 'time_sp', 'r+')
        self._poll = poll()
        self._poll.register(self._state.attr.fileno(), _POLLPRI)
        self._port = port
        self.RPM = 100 * self._max_speed / self._count_per_rot / 60
        self.DPS = self.RPM / 6
        self._command.write('reset')
        # ramping seems to be broken in the kernel drivers
        # self._ramp_up_sp.write(100)
        # self._ramp_down_sp.write(100)

    def on(self, speed):
        """Run the motor at the specified speed.

        The motor will continue to run at this speed until another command is
        given.

        Parameters:
            speed (int): The target speed in percent [-100..100].
        """
        self._set_speed_sp(speed)
        self._command.write('run-forever')

    def on_for_degrees(self, speed, degrees, brake=True):
        """Run the motor at the target speed for the specified number of
        degrees.

        The motor will run until the new position is reached or another command
        is given.

        Parameters:
            speed (int): The target speed in percent [-100..100].
            degrees (int): The number of degrees to turn the motor.
            brake (bool): ``True`` cases the motor to hold it's position when
                          when it is reached. ``False`` will let the motor
                          coast to a stop.
        """
        # driver uses absolute value of speed, so we have to invert degrees
        # to make it work as expected
        if speed < 0:
            degrees *= -1
        self._set_speed_sp(speed)
        self._set_stop_action(brake and 'hold' or 'coast')
        self._set_position_sp(degrees)
        self._command.write('run-to-rel-pos')
        self._wait()

    def on_for_rotations(self, speed, rotations, brake=True):
        """Run the motor at the target speed for the specified number of
        rotations.

        The motor will run until the new position is reached or another command
        is given.

        Parameters:
            speed (int): The target speed in percent [-100..100].
            rotations (float): The number of rotations to turn the motor.
            brake (bool): ``True`` causes the motor to hold it's position when
                when it is reached. ``False`` will let the motor coast to a
                stop.
        """
        self.on_for_degrees(speed, rotations * 360, brake)

    def on_for_time(self, speed, time, brake=True):
        """Run the motor at the target speed for a fixed duration.

        The motor will run until the time expires or another command is given.

        Parameters:
            speed (int): The target speed in percent [-100..100].
            time (float): The time for the motor to run in seconds.
            brake (bool): ``True`` causes the motor to hold it's position when
                when it is reached. ``False`` will let the motor coast to a
                stop.
        """
        self._set_speed_sp(speed)
        self._set_time_sp(int(time * 1000))
        self._set_stop_action(brake and 'hold' or 'coast')
        self._command.write('run-timed')
        self._wait()

    def on_unregulated(self, duty_cycle):
        """Run the motor using the specified duty cycle.

        The motor will continue to run with this duty cycle another command is
        given.

        Parameters:
            duty_cycle (int): the duty cycle, -100 to 100 percent
        """
        self._set_duty_cycle_sp(duty_cycle)
        self._command.write('run-direct')

    def off(self, brake=True):
        """Stop the motor

        Parameters:
            brake (bool): ``True`` cases the motor to hold it's position when
                when it is reached. ``False`` will let the motor coast to a
                stop.
        """
        self._set_stop_action(brake and 'hold' or 'coast')
        self._command.write('stop')

    def _wait(self):
        """Wait for the run command to complete."""
        while True:
            state = self._state.read()
            if 'running' not in state or 'holding' in state:
                break
            while True:
                try:
                    self._poll.poll()
                    break
                except OSError as err:
                    if err.args[0] == EINTR:
                        continue
                    raise

    def _set_duty_cycle_sp(self, duty_cycle):
        if duty_cycle < -100 or duty_cycle > 100:
            raise ValueError('duty cycle is out of range')
        self._duty_cycle_sp.write(duty_cycle)

    def _set_speed_sp(self, speed):
        # convert speed from % to tacho counts per second
        if speed < -100 or speed > 100:
            raise ValueError('speed is out of range')
        speed = int(speed * self._max_speed / 100)
        self._speed_sp.write(speed)

    def _set_position_sp(self, degrees):
        # convert rotations to tacho counts
        counts = int(self._count_per_rot * degrees / 360)
        self._position_sp.write(counts)

    def _set_time_sp(self, time):
        if time < 0:
            raise ValueError('time is out of range')
        self._time_sp.write(time)

    def _set_stop_action(self, action):
        if action not in self._stop_actions:
            raise ValueError('Invalid stop action')
        self._stop_action.write(action)


class LargeMotor(Motor):
    """Object that represents a LEGO EV3 Large motor.

    Parameters:
        port (str): The output port the motor is connected to, ``A``, ``B``,
            ``C`` or ``D``.
    """

    def __init__(self, port):
        """Create a new instace of a large motor.
        """
        super(LargeMotor, self).__init__(port, 'lego-ev3-l-motor')


class MediumMotor(Motor):
    """Object that represents a LEGO EV3 Medium motor.

    Parameters:
        port (str): The output port the motor is connected to, ``A``, ``B``,
            ``C`` or ``D``.
    """

    def __init__(self, port):
        """Create a new instace of a medium motor.
        """
        super(MediumMotor, self).__init__(port, 'lego-ev3-m-motor')


class Steer():
    """Object that represents a two LEGO EV3 Large motors, used in a steering
    configuration.

    Parameters:
        left_port (str): The output port the left motor is connected to,
            ``A``, ``B``, ``C`` or ``D``.
        right_port (str): The output port the left motor is connected to.
    """
    def __init__(self, left_port, right_port):
        self._left_motor = LargeMotor(left_port)
        self._right_motor = LargeMotor(right_port)

    def on_for_degrees(self, steering, speed, degrees, brake=True):
        """Run the motors at the target speed for the specified number of
        degrees.

        The motors will run until the new position is reached or another
        command is given.

        Parameters:
            steering (int): The direction to steer [-100..100]. Positive values
                will turn left and negative values will turn right.
            speed (int): The target speed in percent [-100..100].
            degrees (int): The number of degrees to turn the motors.
            brake (bool): ``True`` cases the motor to hold it's position when
                          when it is reached. ``False`` will let the motor
                          coast to a stop.
        """
        if steering > 100 or steering < -100:
            raise ValueError('steering is out of range')
        if speed < 0:
            speed = abs(speed)
            degrees *= -1
        left_speed = right_speed = speed
        left_degrees = right_degrees = int(degrees)
        if steering < 0:
            steering = (50 + steering) * 2
            left_speed = speed * steering / 100
            left_degrees = degrees * steering / 100
        elif steering > 0:
            steering = (50 - steering) * 2
            right_speed = speed * steering / 100
            right_degrees = degrees * steering / 100

        stop_action = brake and 'hold' or 'coast'

        self._left_motor._set_speed_sp(left_speed)
        self._left_motor._set_position_sp(left_degrees)
        self._left_motor._set_stop_action(stop_action)
        self._right_motor._set_speed_sp(right_speed)
        self._right_motor._set_position_sp(right_degrees)
        self._right_motor._set_stop_action(stop_action)

        if left_degrees:
            self._left_motor._command.write('run-to-rel-pos')
        else:
            self._left_motor._command.write('stop')

        if right_degrees:
            self._right_motor._command.write('run-to-rel-pos')
        else:
            self._right_motor._command.write('stop')

        self._left_motor._wait()
        self._right_motor._wait()

    def on_for_rotations(self, steering, speed, rotations, brake=True):
        """Run the motors at the target speed for the specified number of
        rotations.

        The motors will run until the new position is reached or another
        command is given.

        Parameters:
            steering (int): The direction to steer [-100..100]. Positive values
                will turn left and negative values will turn right.
            speed (int): The target speed in percent [-100..100].
            rotations (int): The number of rotations to turn the motors.
            brake (bool): ``True`` cases the motor to hold it's position when
                          when it is reached. ``False`` will let the motor
                          coast to a stop.
        """
        self.on_for_degrees(steering, speed, rotations * 360, brake)


class Tank():
    """Object that represents a two LEGO EV3 Large motors, used in a tank
    drive configuration.

    Parameters:
        left_port (str): The output port the left motor is connected to,
            ``A``, ``B``, ``C`` or ``D``.
        right_port (str): The output port the left motor is connected to.
    """
    def __init__(self, left_port, right_port):
        self._steer = Steer(left_port, right_port)

    def on_for_degrees(self, left_speed, right_speed, degrees, brake=True):
        """Run the motors at the target speeds for the specified number of
        degrees.

        The motors will run until the new position is reached or another
        command is given.

        Parameters:
            left_speed (int): The target speed for the left motor in percent
                [-100..100].
            right_speed (int): The target speed for the right motor in percent
                [-100..100].
            degrees (int): The number of degrees to turn the motors.
            brake (bool): ``True`` cases the motor to hold it's position when
                          when it is reached. ``False`` will let the motor
                          coast to a stop.
        """
        if left_speed < -100 or left_speed > 100:
            raise ValueError('left_speed is out of range')
        if right_speed < -100 or right_speed > 100:
            raise ValueError('right_speed is out of range')

        # algorithm based on EV3-G tank block
        if degrees < 0:
            left_speed *= -1
            right_speed *= -1
            degrees = abs(degrees)
        if abs(left_speed) > abs(right_speed):
            speed = left_speed
        else:
            speed = right_speed

        if speed:
            turn = 50 * (left_speed - right_speed) / speed
        else:
            turn = 0

        self._steer.on_for_degrees(turn, speed, degrees, brake)

    def on_for_rotations(self, left_speed, right_speed, rotations, brake=True):
        """Run the motors at the target speeds for the specified number of
        rotations.

        The motors will run until the new position is reached or another
        command is given.

        Parameters:
            left_speed (int): The target speed for the left motor in percent
                [-100..100].
            right_speed (int): The target speed for the right motor in percent
                [-100..100].
            rotations (int): The number of rotations to turn the motors.
            brake (bool): ``True`` cases the motor to hold it's position when
                          when it is reached. ``False`` will let the motor
                          coast to a stop.
        """
        self.on_for_degrees(left_speed, right_speed, rotations * 360, brake)
