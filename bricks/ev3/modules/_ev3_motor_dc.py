from pybricks.ev3devices import Motor
from pybricks.parameters import Port


motors = [None, None, None, None]
ports = [Port.A, Port.B, Port.C, Port.D]


def motor_set_dc(index: int, dc: int):
    """Sets the DC value of a motor if it is plugged in.

    Returns the given DC value if a nonzero value was set, else 0.
    """

    # If there was a motor, close if it got unplugged.
    if motors[index] is not None:
        try:
            motors[index].angle()
        except OSError:
            motors[index].close()
            motors[index] = None
            pass

    # If there isn't a motor, try to initialize it now.
    if motors[index] is None:
        try:
            motors[index] = Motor(ports[index])
        except OSError:
            pass

    # Drive the motor if there is one.
    if motors[index]:
        try:
            if dc:
                motors[index].dc(dc)
                return dc
            else:
                motors[index].stop()
        except OSError:
            motors[index].close()
            motors[index] = None

    return 0
