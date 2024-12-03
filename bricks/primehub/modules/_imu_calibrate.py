from pybricks.hubs import ThisHub
from pybricks.parameters import Side, Axis
from pybricks.tools import wait, vector


hub = ThisHub()


def beep(freq):
    try:
        hub.speaker.beep(freq, 100)
    except AttributeError:
        # Technic hub does not have a speaker.
        pass
    wait(10)


def wait_for_stationary(side):
    while not hub.imu.stationary() or hub.imu.up() != side:
        wait(10)


up_sides = {
    Side.FRONT: (0, 0),
    Side.BACK: (1, 0),
    Side.LEFT: (2, 1),
    Side.RIGHT: (3, 1),
    Side.TOP: (4, 2),
    Side.BOTTOM: (5, 2),
}

gravity = [0] * 6
bias = vector(0, 0, 0)

STATIONARY_COUNT = 1000


def roll_over_axis(axis, new_side):

    global bias, bias_count

    print("Roll it towards you, without lifting the hub up!")

    angle_start = hub.imu.rotation(axis, calibrated=False)
    while hub.imu.up() != new_side or not hub.imu.stationary():

        _, _, z = hub.imu.orientation() * axis
        if abs(z) > 0.07:
            print(hub.imu.orientation() * axis)
            raise RuntimeError("Lifted it!")
        wait(100)

    uncalibrated_90_deg_rotation = abs(hub.imu.rotation(axis, calibrated=False) - angle_start)
    if abs(uncalibrated_90_deg_rotation - 90) > 10:
        raise RuntimeError("Not 90 deg!")

    print("Calibrating...")
    beep(1000)

    rotation_start = vector(
        hub.imu.rotation(Axis.X, calibrated=False),
        hub.imu.rotation(Axis.Y, calibrated=False),
        hub.imu.rotation(Axis.Z, calibrated=False),
    )

    acceleration = vector(0, 0, 0)

    for i in range(STATIONARY_COUNT):
        acceleration += hub.imu.acceleration(calibrated=False)
        bias += hub.imu.angular_velocity(calibrated=False)
        wait(1)

    acceleration /= STATIONARY_COUNT

    rotation_end = vector(
        hub.imu.rotation(Axis.X, calibrated=False),
        hub.imu.rotation(Axis.Y, calibrated=False),
        hub.imu.rotation(Axis.Z, calibrated=False),
    )
    if abs(rotation_end - rotation_start) > 1:
        raise RuntimeError("Moved it!")

    side_index, axis_index = up_sides[new_side]

    # Store the gravity value for the current side being up.
    # We will visit each side several times. We'll divide by the number
    # of visits later.
    gravity[side_index] += acceleration[axis_index]

    beep(500)

    return uncalibrated_90_deg_rotation


calibrate_x = """
Going to calibrate X now!
- Put the hub on the table in front of you.
- top side (display) facing up
- right side (ports BDF) towards you.
"""

calibrate_y = """
Going to calibrate Y now
- Put the hub on the table in front of you.
- top side (display) facing up
- back side (speaker) towards you.
"""

calibrate_z = """
Going to calibrate Z now!
- Put the hub on the table in front of you.
- front side (USB port) facing up
- left side (ports ACE) towards you
"""

REPEAT = 2

# For each 3-axis run, we will visit each side twice.
SIDE_COUNT = REPEAT * 2


def roll_hub(axis, message, start_side, sides):
    print(message)
    wait_for_stationary(start_side)
    beep(500)
    rotation = 0
    for _ in range(REPEAT):
        for side in sides:
            rotation += roll_over_axis(axis, side)
    return rotation / REPEAT


rotation_x = roll_hub(
    Axis.X, calibrate_x, Side.TOP, [Side.LEFT, Side.BOTTOM, Side.RIGHT, Side.TOP]
)
rotation_y = roll_hub(
    Axis.Y, calibrate_y, Side.TOP, [Side.FRONT, Side.BOTTOM, Side.BACK, Side.TOP]
)
rotation_z = roll_hub(
    Axis.Z, calibrate_z, Side.FRONT, [Side.RIGHT, Side.BACK, Side.LEFT, Side.FRONT]
)

hub.imu.settings(
    angular_velocity_bias=tuple(bias / SIDE_COUNT / STATIONARY_COUNT / 6),
    angular_velocity_scale=(rotation_x, rotation_y, rotation_z),
    acceleration_correction=[g / SIDE_COUNT for g in gravity],
)

print("Result: ", hub.imu.settings())
