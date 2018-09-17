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

# Print angle and debug run command (required encoders are enabled)
print(motor.angle())
motor.run(500)
sleep(5)
