from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks import version

print(version)

# Initialize the fronts.
steer = Motor(Port.C)
front = Motor(Port.A, Direction.COUNTERCLOCKWISE)
rear = Motor(Port.B, Direction.COUNTERCLOCKWISE)

front.control.limits(acceleration=1000)
rear.control.limits(acceleration=1000)

# Allocate the data logs.
DURATION = 8000
DIV = 6
front.log.start(DURATION, DIV)
front.control.log.start(DURATION, DIV)

# Run the front.
front.run(1000)
rear.run(1000)
wait(2000)
front.run(0)
rear.run(0)
wait(2000)
front.run(-1000)
rear.run(-1000)
wait(2000)
front.run(0)
rear.run(0)
wait(2000)

# Transfer data logs.
print("Transferring data...")
front.log.save("servo.txt")
front.control.log.save("control.txt")
print("Done")
