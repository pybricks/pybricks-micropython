from pybricks.pupdevices import Motor
from pybricks.parameters import Direction, Port
from pybricks.robotics import DriveBase
from pybricks.tools import wait

# Initialize the drive base.
motorL = Motor(Port.A, Direction.COUNTERCLOCKWISE)
motorR = Motor(Port.B)
drive_base = DriveBase(motorL, motorR, wheel_diameter=56, axle_track=80)

# Demo 1: Using straight() to move Fwd & Rev in mm
drive_base.straight(200, wait=False)
wait(500)
assert motorL.speed() > 0, "When driving Fwd in straight line, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in straight line, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(250)

drive_base.straight(-200, wait=False)
wait(500)
assert motorL.speed() < 0, "When driving Rev in straight line, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in straight line, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(250)

# Demo 2: Using turn() for in-place turns by angle (+ve = CW, -ve = CCW)
drive_base.turn(90, wait=False)
wait(500)
assert motorL.speed() > 0, "For CW in-place turn 90 degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW in-place turn 90 degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(180, wait=False)
wait(500)
assert motorL.speed() > 0, "For CW in-place turn 180 degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW in-place turn 180 degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(-270, wait=False)
wait(500)
assert motorL.speed() < 0, "For CCW in-place turn 270 degrees, left speed should be -ve"
assert motorR.speed() > 0, "For CCW in-place turn 270 degrees, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(1080, wait=False)
wait(500)
assert motorL.speed() > 0, "For CW in-place turn 1080 degrees, left speed should be +ve"
assert motorR.speed() < 0, "For CW in-place turn 1080 degrees, right speed should be -ve"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.turn(-1440, wait=False)
wait(500)
assert motorL.speed() < 0, "For CCW in-place turn 1440 degrees, left speed should be -ve"
assert motorR.speed() > 0, "For CCW in-place turn 1440 degrees, right speed should be +ve"
while not drive_base.done():
    wait(10)
wait(500)


# Demo 3: Using curve() drives the 'center point' between the wheels a full
# circle (360 degrees or part thereof) around a circle of 12cm radius.
drive_base.curve(120, 360, wait=False)  # Drives forward along circle to robot's right
wait(500)
assert motorL.speed() > 0, "When driving Fwd in CW Curve, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in CW Curve, right speed should be +ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Fwd in CW Curve, motorL should be greater than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(-140, 360, wait=False)  # Drives backward along circle to robot's right
wait(500)
assert motorL.speed() < 0, "When driving Rev in CCW Curve, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in CCW Curve, right speed should be -ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Rev in CCW Curve, motorL should be less (i.e. faster) than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(120, -360, wait=False)  # Drives forward along circle to robot's left
wait(500)
assert motorL.speed() > 0, "When driving Fwd in CCW Curve, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in CCW Curve, right speed should be +ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Fwd in CCW Curve, motorL should be less than motorR"
while not drive_base.done():
    wait(10)
wait(500)

drive_base.curve(-120, -360, wait=False)  # Drives in reverse along circle to robot's left
wait(500)
assert motorL.speed() < 0, "When driving Rev in CW Curve, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in CW Curve, right speed should be -ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Rev in CW Curve, motorL should be greater than motorR"
while not drive_base.done():
    wait(10)
wait(500)

# Demo 4: Using drive() to control the robot in various ways.
drive_base.drive(200, 0)  # Drive straight Fwd
wait(1000)
assert motorL.speed() > 0, "When driving Fwd in straight line, left speed should be +ve"
assert motorR.speed() > 0, "When driving Fwd in straight line, right speed should be +ve"
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(-200, 0)  # Drive straight Rev
wait(1000)
assert motorL.speed() < 0, "When driving Rev in straight line, left speed should be -ve"
assert motorR.speed() < 0, "When driving Rev in straight line, right speed should be -ve"
wait(500)
drive_base.stop()
wait(250)

drive_base.drive(200, 90)  # Drives CW Fwd
wait(1000)
assert motorL.speed() > 0, "When driving CW Curve Fwd, left speed should be +ve"
assert motorR.speed() > 0, "When driving CW Curve Fwd, right speed should be +ve"
assert (
    motorL.speed() > motorR.speed()
), "When driving Fwd in CW Curve, motorL should be greater than motorR"
wait(3000)  # 4 seconds x 90 deg/s approx full circle
drive_base.stop()
wait(250)

drive_base.drive(-200, 90)  # Drives CW Rev
wait(1000)
assert motorL.speed() < 0, "When driving CCW Curve Rev, left speed should be -ve"
assert motorR.speed() < 0, "When driving CCW Curve Rev, right speed should be -ve"
assert (
    motorR.speed() < motorL.speed()
), "When driving Rev in CCW Curve, motorR should be less than motorL"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)

drive_base.drive(200, -90)  # Drives CCW Fwd
wait(1000)
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
assert motorL.speed() < 0, "When driving CW Curve Rev, left speed should be +ve"
assert motorR.speed() < 0, "When driving CW Curve Rev, right speed should be +ve"
assert (
    motorL.speed() < motorR.speed()
), "When driving Rev in CW Curve, motorL should be less than motorR"
wait(3000)  # 4 seconds x 90 deg/s = full circle
drive_base.stop()
wait(250)
