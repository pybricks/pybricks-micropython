from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
DURATION = 12000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

for i in range(11):
    motor.dc(i * 10)
    wait(1000)

# Wait so we can also log the stopped behavior.
motor.dc(0)
wait(500)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
