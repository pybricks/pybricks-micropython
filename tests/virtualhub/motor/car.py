from pybricks.parameters import Direction, Port
from pybricks.pupdevices import Motor
from pybricks.robotics import Car
from pybricks.tools import wait

# Set up all devices.
wait(2000)
front = Motor(Port.A, Direction.CLOCKWISE)
rear = Motor(Port.B, Direction.CLOCKWISE)
steer = Motor(Port.C, Direction.CLOCKWISE)
car = Car(steer, [front, rear])


car.drive_power(50)
wait(1000)

car.steer(100)
wait(2000)

car.steer(-100)
car.drive_power(-100)
wait(2000)
