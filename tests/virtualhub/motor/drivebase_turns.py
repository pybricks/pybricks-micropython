from pybricks.pupdevices import Motor
from pybricks.parameters import Direction, Port
from pybricks.robotics import DriveBase
from pybricks.tools import wait

# Initialize the drive base.
motorL = Motor(Port.C, Direction.COUNTERCLOCKWISE)
motorR = Motor(Port.D)
drive_base = DriveBase(motorL, motorR, wheel_diameter=56, axle_track=80)

# User's can override the default speeds with .setting()  (Defaults are 40% of it's max speed)
# .settings(straight_speed, straight_acceleration, turn_rate, turn_acceleration)

# Demo 1 - Using .straight() to move Fwd & Rev in mm
# .straight(distance, then=Stop.HOLD, wait=True)
drive_base.straight(200, wait=False)
wait(500)
print(
    "Test 1 - Drive Fwd - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving Fwd in straight line, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in straight line, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(250)

drive_base.straight(-200, wait=False)
wait(500)
print(
    "Test 2 -Drive Rev - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving Rev in straight line, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in straight line, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(250)

# Demo 2 - Using .turn() to 'Point Turn' by angle (+ve = CW, -ve = CCW)
# .turn(angle, then=Stop.HOLD, wait=True)
drive_base.turn(90, wait=False)
wait(500)
print(
    "Test 3 - CW Point Turn 90 degrees - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "For CW Point Turn 90 Degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW Point Turn 90 Degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(180, wait=False)
wait(500)
print(
    "Test 4 - CW Point Turn 180 degrees - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "For CW Point Turn 180 Degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW Point Turn 180 Degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(-270, wait=False)
wait(500)
print(
    "Test 5 - CCW Point Turn 270 degrees - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "For CCW Point Turn 270 Degrees, left speed should be -ve"
assert motorR.speed() > 0, "For CCW Point Turn 270 Degrees, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(1080, wait=False)
wait(500)
print(
    "Test 6 - CW Point Turn 1080 degrees - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "For CW Point Turn 1080 Degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW Point Turn 1080 Degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(-1440, wait=False)
wait(500)
print(
    "Test 7 - CCW Point Turn 1440 degrees - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "For CCW Point Turn 270 Degrees, left speed should be -ve"
assert motorR.speed() > 0, "For CCW Point Turn 270 Degrees, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(500)


# Demo 3 - Using .curve() drives the 'center point' between the wheels a full circle (360 degrees or part thereof) around a circle of 12cm radius
# .curve(radius, angle, then=Stop.HOLD, wait=True)
drive_base.curve(120, 360, wait=False)  # Drives Clockwise Fwd
wait(500)
print(
    "Test 8 - CW Curve Fwd (radius 12cm) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving Fwd in CW Curve, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in CW Curve, right speed should be +ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Fwd in CW Curve, motorL should be greater than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(-140, 360, wait=False)  # Drives CounterClockwise in Rev
wait(500)
print(
    "Test 9 - CCW Curve Rev (radius 12cm) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving Rev in CCW Curve, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in CCW Curve, right speed should be -ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Rev in CCW Curve, motorL should be less than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(120, -360, wait=False)  # Drives Counterclockwise Fwd
wait(500)
print(
    "Test 10 - CCW Curve Fwd (radius 12cm) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving Fwd in CCW Curve, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in CCW Curve, right speed should be +ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Fwd in CCW Curve, motorL should be less than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(-120, -360, wait=False)  # Drives Clockwise in Rev
wait(500)
print(
    "Test 11 - CW Curve Rev (radius 12cm) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving Rev in CW Curve, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in CW Curve, right speed should be -ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Rev in CW Curve, motorL should be greater than motorR"
while not drive_base.done():
    wait(10)
wait(500)

# Demo 4 - Using .drive() to control the robot in various ways
# .drive(speed, turn_rate)
drive_base.drive(200, 0)  # Drive Fwd
wait(1000)
print(
    "Test 12 - .drive() - Drive Fwd - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving Fwd in straight line, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in straight line, right speed should be +ve"
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(-200, 0)  # Drive Rev
wait(1000)
print(
    "Test 13 - .drive() - Drive Rev - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving Rev in straight line, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in straight line, right speed should be -ve"
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(200, 90)  # Drives CW Fwd
wait(1000)
print(
    "Test 14 - .drive() - CW Curve Fwd (turning 90/sec) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving CW Curve Fwd, left speed should be +ve"
assert motorR.speed() > 0, "When driving CW Curve Fwd, right speed should be +ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Fwd in CW Curve, motorL should be greater than motorR"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(-200, 90)  # In corrected version it should Drive CCW in Rev
wait(1000)
print(
    "Test 15 - .drive() - CCW Curve Rev (turning 90/sec) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving CCW Curve Rev, left speed should be -ve"
assert motorR.speed() < 0, "When driving CCW Curve Rev, right speed should be -ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Rev in CCW Curve, motorL should be less than motorR"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(200, -90)  # In corrected version it should Drive CCW Fwd
wait(1000)
print(
    "Test 16 - .drive() - CCW Curve Fwd (turning 90/sec) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() > 0, "When driving CCW Curve Fwd, left speed should be +ve"
assert motorR.speed() > 0, "When driving CCW Curve Fwd, right speed should be +ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Fwd in CCW Curve, motorL should be less than motorR"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(-200, -90)  # In corrected version it should drive CW in Rev
wait(1000)
print(
    "Test 17 - .drive() - CW Curve Rev (turning 90/sec) - motorL.speed() = {}, motorR.speed() = {}".format(
        motorL.speed(), motorR.speed()
    )
)
assert motorL.speed() < 0, "When driving CW Curve Rev, left speed should be +ve"
assert motorR.speed() < 0, "When driving CW Curve Rev, right speed should be +ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Rev in CW Curve, motorL should be less than motorR"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)
