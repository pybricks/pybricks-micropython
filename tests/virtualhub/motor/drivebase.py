from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase

# Initialize default "Driving Base" with medium motors and wheels.
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)

# Drive straight forward and back again.
drive_base.straight(500)
drive_base.straight(-500)

wait(100)
drive_base.stop()
