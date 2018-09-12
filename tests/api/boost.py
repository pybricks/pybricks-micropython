from _constants import *
from time import sleep

from hub import MovehubMotor

# Initialize the motor
motor = MovehubMotor(PORT_A)

# Move!
print("Start moving.")
motor.duty(50)
sleep(1)

# And stop
print("Stop.")
motor.coast()
