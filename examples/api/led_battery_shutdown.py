from movehub import *
from utime import sleep_ms

# Light
light(Color.yellow)
sleep_ms(1000)
light(Color.purple)

# Battery
print("mV:", battery.voltage())
print("mA:", battery.current())
print(" %:", battery.percent())
if battery.low():
    print("Replace batteries")

# Try one of the following
shutdown()
reboot()
update()
