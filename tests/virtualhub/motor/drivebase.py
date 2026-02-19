from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase
from umath import pi

# Initialize default "Driving Base" with medium motors and wheels.
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)


def expect_state(
    expected_distance, expected_drive_speed, expected_angle, expected_angular_velocity
):
    expected_state = (
        expected_distance,
        expected_drive_speed,
        expected_angle,
        expected_angular_velocity,
    )
    distance, drive_speed, angle, angular_velocity = drive_base.state()
    if (
        abs(expected_distance - distance) > 10
        or abs(expected_drive_speed - drive_speed) > 30
        or abs(expected_angle - angle) > 10
        or abs(expected_angular_velocity - angular_velocity) > 30
    ):
        raise ValueError(
            "Expected {0} but got {1}".format(expected_state, drive_base.state())
        )


# Expect zeroed state on startup.
expect_state(0, 0, 0, 0)

# In the idealized simulation, we should be exactly on target
# after settling for a while.
drive_base.straight(1000)
wait(1000)
assert abs(drive_base.distance() - 1000) <= 3
drive_base.straight(-1000)

# Drive straight forward and back again.
drive_base.straight(500)
expect_state(500, 0, 0, 0)
drive_base.straight(-500)
expect_state(0, 0, 0, 0)

# Curve and back again.
drive_base.curve(100, 90)
expect_state(pi / 2 * 100, 0, 90, 0)
drive_base.curve(-100, 90)
expect_state(0, 0, 0, 0)

# Test setting of distance and angle state.
drive_base.reset(1000, 360)
expect_state(1000, 0, 360, 0)
drive_base.straight(-1000)
drive_base.turn(-360)
expect_state(0, 0, 0, 0)

# Expect to reach target speed.
drive_base.drive(200, 25)
wait(500)
expect_state(drive_base.distance(), 200, drive_base.angle(), 25)

# Expect reset to stop the robot
drive_base.reset()
wait(500)
expect_state(drive_base.distance(), 0, drive_base.angle(), 0)

# Drive fast.
drive_base.settings(
    straight_speed=500,
    straight_acceleration=1000,
    turn_rate=500,
    turn_acceleration=2000,
)
drive_base.straight(500)
