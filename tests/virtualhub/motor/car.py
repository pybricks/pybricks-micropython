from pybricks.parameters import Direction, Port
from pybricks.pupdevices import Motor
from pybricks.robotics import Car
from pybricks.tools import wait

# Set up all devices.
front = Motor(Port.A, Direction.CLOCKWISE)
rear = Motor(Port.B, Direction.CLOCKWISE)
steer = Motor(Port.C, Direction.CLOCKWISE)
car = Car(steer, [front, rear])

# Simulated endstop is at 142 degrees, minus 10 degrees in the car class.
TARGET = 132

car.drive_power(50)
wait(1000)

car.steer(100)
wait(2000)
assert abs(steer.angle() - TARGET) < 5

car.steer(-100)
wait(2000)
assert abs(steer.angle() - -TARGET) < 5

car.steer(0)
wait(2000)
assert abs(steer.angle()) < 5

car.drive_power(-100)
wait(2000)
