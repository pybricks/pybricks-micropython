from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Stop
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
DURATION = 4000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Find the steering endpoint on the left and right. The difference
# between them is the total angle it takes to go from left to right.
# The middle is in between.
left_end = motor.run_until_stalled(-200, then=Stop.HOLD)
right_end = motor.run_until_stalled(200, then=Stop.HOLD)

# We are now at the right limit. We reset the motor angle to the
# limit value, so that the angle is 0 when the steering mechanism is
# centered.
limit = (right_end - left_end) // 2
motor.reset_angle(limit)
motor.run_target(speed=200, target_angle=0)

# Wait so we can also log the stopped behavior.
wait(500)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
