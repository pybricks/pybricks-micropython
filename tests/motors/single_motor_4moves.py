from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Stop
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
DURATION = 10 * 1000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Run the motor.
motor.run_target(500, 360)
motor.run_target(500, -360)
motor.run_target(1000, 360, then=Stop.COAST)
motor.run_target(1000, -360)

# Wait so we can also log the stopped behavior.
wait(1000)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
