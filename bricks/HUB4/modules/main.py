
test = Motor(Port.B)
test.dc(50)
wait(1000)
test.dc(0)

try:
    hub.light(Color.GREEN)
    sensor = ColorDistanceSensor(Port.A)
    sensor.ambient()
    wait(1000)
    sensor.color()
    wait(1000)
except:
    hub.light(Color.RED)
    wait(1000)
    hub.shutdown()
    wait(1000)
