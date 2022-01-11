from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Stop
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.D)

# Allocate the data logs.
DURATION = 10 * 1000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Run the motor against a mechanical endstop so it stalls.
motor.run_target(500, -360, wait=False)
wait(1000)

motor.track_target(90)
wait(2000)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
