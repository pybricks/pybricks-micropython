"""API usage example for LEGO MINDSTORMS EV3."""

# Import basic stuff. We could perhaps import brick and all relevant ports by default like on the pyboard
import brick
from port import PORT_B, PORT_C, PORT_1

# Import the devices we want to use
from ev3devices import LargeMotor, TouchSensor

# Import other modules
import sleep
from robotics import DriveBase

# Initialize the devices
left_motor = LargeMotor(PORT_B)
right_motor = LargeMotor(PORT_C)
bumper = TouchSensor(PORT_1)
base = DriveBase(left_motor, right_motor, wheel_diameter=4, axle_track=14)

# Drive robot straight ahead and stop at an obstacle
base.drive(4, 0)
while not bumper.pressed():
    sleep(0.1)
base.stop()

# Play a sound on the brick
brick.beep()
