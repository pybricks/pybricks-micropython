from pybricks.pupdevices import Motor
from pybricks.tools import StopWatch
from pybricks.parameters import Port
from pybricks import version

from umath import sin, pi

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate space for data log samples.
DURATION = 4000
motor.log.start(DURATION)

# Start the motor at 500 degrees per second.
motor.run(speed=500)

# Create garbage data for a few seconds.
watch = StopWatch()
while watch.time() < DURATION:
    garbage = [sin(watch.time() / DURATION * 2 * pi)] * 50

# Stop the motor.
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
print("Done")
