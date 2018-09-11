from ev3devices import LargeMotor
from time import sleep

# Until we decide how to deal with port constants etc
# this is a test with one flat module with constants
from _constants import *

# Configure a large EV3 motor, set duty to 30, and turn it off
motor = LargeMotor(PORT_A, DIR_INVERTED)
print("angle: ", motor.angle())
motor.duty(30)
sleep(1)
print("speed: ", motor.speed())
print("angle: ", motor.angle())
motor.reset_angle(-90)
motor.coast()
print("angle: ", motor.angle())

motor.run(500)
sleep(0.5)
motor.stop(STOP_HOLD, True)
motor.run_time(500, 3, STOP_HOLD, True)
motor.run_angle(500, 90, STOP_HOLD, True)
motor.run_target(500, -360, STOP_HOLD, True)
motor.track_target(500)

