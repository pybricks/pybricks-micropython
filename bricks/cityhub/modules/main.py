
test = Motor(Port.B)
test.dc(50)
wait(1000)
test.dc(0)

try:
    hub.light(Color.GREEN)
    sensor = ColorDistanceSensor(Port.A)

    while True:
        if sensor.reflection() < 40:
            test.dc(20)
        else:
            test.dc(-20)
except:
    hub.light(Color.RED)
    wait(1000)
    hub.shutdown()
    wait(1000)
