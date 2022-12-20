from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
DURATION = 16000
motor.log.start(DURATION, 2)

wait(5000)

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
print("Done")
