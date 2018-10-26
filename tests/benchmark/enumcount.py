# init
from hub import *
from devices import *
from utime import *
motor = MovehubMotor(Port.A)

# main
count = 0
time_now = ticks_ms()
for i in range(0, 1000):
    motor.coast()
    # count += 1
    # count += Stop.brake

time_end = ticks_ms()
print("usec/loop:", time_end-time_now) # 79 us/loop as of 817bc7b
