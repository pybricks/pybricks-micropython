from pybricks.tools import wait
from pybricks.nxtdevices import Motor
from pybricks.parameters import Port

motor = Motor(Port.A)

print("1")

motor.dc(50)
wait(1000)
motor.stop()

print("2")
