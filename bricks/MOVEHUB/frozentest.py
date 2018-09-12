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
motor.coast()

# Print the angle
print("Stopped at:", motor.angle())

# Don't restart immediately
print("\n\n-----Restarting frozen code in 5 seconds-----\n\n")
sleep(5)
