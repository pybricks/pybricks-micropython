from ev3devices import LargeMotor
from time import sleep

# Until we decide how to deal with port constants etc
PORT_A = const(65)
PORT_D = const(68)
STOP_COAST = const(0)
STOP_BRAKE = const(1)
STOP_HOLD = const(2)

# Configure large EV3 motors
left = LargeMotor(PORT_A)
right = LargeMotor(PORT_D)

# A simple 180 maneuver
left.run_target(500, 180, STOP_COAST, True)

# An additional 90 degree maneuver
left.run_angle(500, 90, STOP_BRAKE, True)

# Back to the start
left.run_target(500, 0, STOP_HOLD, True)

# run both motors
left.run_time(500, 3000, STOP_BRAKE, False)
right.run_time(500, 3000, STOP_BRAKE, True)

# Run then hard stop
left.run(500)
sleep(1)
left.stop(False, STOP_HOLD, True)
