from pybricks.pupdevices import Motor
from pybricks.tools import wait, StopWatch
from pybricks.parameters import Port
from pybricks import version

from umath import sin, pi

print(version)

# Initialize the motor and allocate logs.
motor = Motor(Port.A)
DURATION = 4000
motor.log.start(DURATION)

# Move the motor with a speed in a sine pattern.
watch = StopWatch()

while watch.time() < DURATION + 500:

    # Compute sine angle.
    phase = watch.time() / DURATION * 2 * pi
    speed = sin(phase) * 750

    # Set the motor speed to sine value.
    motor.run(speed)

motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
print("Done")
