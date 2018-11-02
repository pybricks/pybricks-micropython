left = MovehubMotor(Port.A)
right = MovehubMotor(Port.B)
sensor = ColorAndDistSensor(Port.D)

# TODO: make a fake drivebase instead of using raw duty
mean = 70
diff = 30

while True:
    if sensor.color() == Color.white:
        left.duty(mean + diff)
        right.duty(mean - diff)
    else:
        left.duty(mean - diff)
        right.duty(mean + diff)
    wait(10)
