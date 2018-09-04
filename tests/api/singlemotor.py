from ev3devices import LargeMotor
from time import sleep

# Until we decide how to deal with port constants etc
port = ord('A')

# Configure a large EV3 motor, set duty to 30, and turn it off
mtr = LargeMotor(port)
mtr.duty(30)
sleep(2)
mtr.coast()
