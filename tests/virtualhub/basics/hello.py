from pybricks.tools import wait
from pybricks.pupdevices import Motor
from pybricks.parameters import Port

motor = Motor(Port.A)

motor.run(200)
print("\nHello")
wait(1000)
print("World\n")
