from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
DURATION = 4000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Run the motor.
motor.run_target(500, 360)

# Wait so we can also log the stopped behavior.
wait(500)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
