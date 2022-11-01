from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase
from pybricks import version

print(version)

# Initialize default "Driving Base" with medium motors and wheels.
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)

# Allocate logs for motors and controller signals.
DURATION = 6000
left_motor.log.start(DURATION)
right_motor.log.start(DURATION)
drive_base.distance_control.log.start(DURATION)
drive_base.heading_control.log.start(DURATION)

# Drive straight forward and back again.
drive_base.curve(radius=500, angle=360)

# Wait so we can also log hold capability, then turn off the motor completely.
wait(100)
drive_base.stop()

# Transfer data logs.
print("Transferring data...")
left_motor.log.save("servo_left.txt")
right_motor.log.save("servo_right.txt")
drive_base.distance_control.log.save("control_distance.txt")
drive_base.heading_control.log.save("control_heading.txt")
print("Done")
