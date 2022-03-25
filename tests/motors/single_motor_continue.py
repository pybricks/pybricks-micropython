from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Stop
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.C)

# Allocate the data logs.
DURATION = 6000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Start from 0 so the 1000 endpoint is easy to see.
motor.reset_angle(0)

# Run the motor 334 degrees and stop.
motor.run_angle(500, 334)

# Run motor 333 degrees and keep going.
motor.run_angle(500, 333, then=Stop.NONE)

# Run motor 333 degrees and stop.
motor.run_angle(500, 333)

# Wait so we can also log the stopped behavior.
wait(500)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
