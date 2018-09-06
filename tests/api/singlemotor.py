from ev3devices import LargeMotor
from time import sleep

# Until we decide how to deal with port constants etc
# this is a test with one flat module with constants
from _constants import *

# Configure a large EV3 motor, set duty to 30, and turn it off
mtr = LargeMotor(PORT_A)
mtr.duty(30)
sleep(2)
mtr.coast()
