from movehub import *

# Light
light(Color.yellow)
wait(1000)
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
