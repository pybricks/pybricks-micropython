from pybricks.pupdevices import Motor
from pybricks.parameters import Direction, Port
from pybricks.robotics import DriveBase
from pybricks.tools import wait

# Initialize the drive base.
motorL = Motor(Port.A, Direction.COUNTERCLOCKWISE)
motorR = Motor(Port.B)
drive_base = DriveBase(motorL, motorR, wheel_diameter=56, axle_track=80)

STARTUP_DELAY = 500

# Demo 4: Using drive() to control the robot in various ways.
drive_base.drive(200, 0)  # Drive straight Fwd
wait(STARTUP_DELAY)
assert motorL.speed() > 0, "When driving Fwd in straight line, left speed should be +ve"
assert motorR.speed() > 0, (
    "When driving Fwd in straight line, right speed should be +ve"
)
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(-200, 0)  # Drive straight Rev
wait(STARTUP_DELAY)
assert motorL.speed() < 0, "When driving Rev in straight line, left speed should be -ve"
assert motorR.speed() < 0, (
    "When driving Rev in straight line, right speed should be -ve"
)
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(200, 90)  # Drives CW Fwd
wait(STARTUP_DELAY)
assert motorL.speed() > 0, "When driving CW Curve Fwd, left speed should be +ve"
assert motorR.speed() > 0, "When driving CW Curve Fwd, right speed should be +ve"
assert motorL.speed() > motorR.speed(), (
    "When driving Fwd in CW Curve, motorL should be greater than motorR"
)
wait(3000)  # 4 seconds x 90 deg/s approx full circle
drive_base.stop()
wait(250)

drive_base.drive(-200, 90)  # Drives CW Rev
wait(STARTUP_DELAY)
assert motorL.speed() < 0, "When driving CCW Curve Rev, left speed should be -ve"
assert motorR.speed() < 0, "When driving CCW Curve Rev, right speed should be -ve"
assert motorR.speed() < motorL.speed(), (
    "When driving Rev in CCW Curve, motorR should be less than motorL"
)
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(200, -90)  # Drives CCW Fwd
wait(STARTUP_DELAY)
assert motorL.speed() > 0, "When driving CCW Curve Fwd, left speed should be +ve"
assert motorR.speed() > 0, "When driving CCW Curve Fwd, right speed should be +ve"
assert motorL.speed() < motorR.speed(), (
    "When driving Fwd in CCW Curve, motorL should be less than motorR"
)
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(-200, -90)  # In corrected version it should drive CW in Rev
wait(STARTUP_DELAY)
assert motorL.speed() < 0, "When driving CW Curve Rev, left speed should be +ve"
assert motorR.speed() < 0, "When driving CW Curve Rev, right speed should be +ve"
assert motorL.speed() < motorR.speed(), (
    "When driving Rev in CW Curve, motorL should be less than motorR"
)
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)
