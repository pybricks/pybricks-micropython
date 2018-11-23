#!/home/robot/pybricks
from boot import *

# Configure motors and sensors
ir = InfraredSensor(Port.S4)
left = LargeMotor(Port.A)
right = LargeMotor(Port.D)

# Setup a stopwatch
watch = StopWatch()

# Begin moving
left.run(500)
right.run(500)

# Wait for infrared sensor
while ir.distance() > 30:
    wait(10)

# Stop
left.stop()
right.stop()

# Get the time we spent driving
elapsed = watch.time()

# Print
print("Drove for", elapsed, "milliseconds.")
