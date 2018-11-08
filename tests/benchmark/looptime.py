# Typical ingredients for a "segway program": Reading sensors and setting duty in a fast loop. Just some random formula for now.

from hub import *
from devices import *
from utime import *

left = MoveHubMotor(Port.A)
right = MoveHubMotor(Port.B)

time_now = ticks_ms()
for i in range(0, 10000):
    avg_pos = left.angle() + right.angle()
    formula = i//100-avg_pos//36
    left.duty(formula)
    right.duty(formula)

time_end = ticks_ms()

left.coast()
right.coast()

print("usec/loop:", (time_end-time_now)//10) # 433 us/loop as of 817bc7b
