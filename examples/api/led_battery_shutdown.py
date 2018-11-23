# hub.light
hub.light(Color.yellow)
wait(1000)
hub.light(Color.purple)

# Battery
print("mV:", hub.battery.voltage())
print("mA:", hub.battery.current())
print(" %:", hub.battery.percent())
if hub.battery.low():
    print("Replace batteries")

# Try one of the following
hub.shutdown()
hub.reboot()
hub.update()
