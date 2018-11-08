left = MovehubMotor(Port.A)
right = MovehubMotor(Port.B)
base = DriveBase(left, right)
sensor = ColorDistanceSensor(Port.D)

while True:
    steering = ((sensor.reflection() - 45)*2)//3
    base.drive(60, steering)
