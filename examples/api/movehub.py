from hub import *
from devices import *
from utime import sleep_ms

# Configure hub motors
left = MovehubMotor(PORT_A)
right = MovehubMotor(PORT_B)

# A simple 180 maneuver to a fixed position
left.run_target(500, 180)

# An additional 90 degree maneuver
left.run_angle(500, 90)

# Back to the start and hold there (See #17 TODO that fixes high pitch due to too low duty)
left.run_target(500, 0, HOLD)

# run both motors (this will become simpler and more precise once we implement the DriveBase class)
left.run_time(500, 3000, BRAKE, BACKGROUND)
right.run_time(500, 3000, BRAKE, WAIT)

# Run then forced stop
left.run(500)
sleep_ms(1000)
left.stop(FAST, BRAKE)
