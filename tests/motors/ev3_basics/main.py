#!/usr/bin/env pybricks-micropython
from pybricks.hubs import EV3Brick
from pybricks.ev3devices import Motor
from pybricks.parameters import Port
from pybricks import version

print(version)

# Create your objects here.
ev3 = EV3Brick()
m = Motor(Port.A)

# Write your program here.
ev3.speaker.beep()
