from hub import *
from devices import *
from time import sleep

# Configure large EV3 motors
left = MovehubMotor(PORT_A)
right = MovehubMotor(PORT_B)

# A simple 180 maneuver
left.run_target(500, 180, COAST, WAIT)

# An additional 90 degree maneuver
left.run_angle(500, 90, BRAKE, WAIT)

# Back to the start
left.run_target(500, 0, HOLD, WAIT)

# run both motors
left.run_time(500, 3000, BRAKE, BACKGROUND)
right.run_time(500, 3000, BRAKE, WAIT)

# Run then forced stop
left.run(500)
sleep(1)
left.stop(FAST, BRAKE, WAIT)
