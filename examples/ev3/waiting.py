#!/home/robot/pybricks
from ev3brick import Port
from ev3devices import *
from robotics import *
from future import wait

# Configure motors and sensors
ir = InfraredSensor(Port.S4)
left = LargeMotor(Port.A)
right = LargeMotor(Port.D)

# Begin moving
left.run(500)
right.run(500)

# Wait for infrared sensor to be less than 50
wait(ir.distance, '<', 50)

# Stop
left.stop()
right.stop()
