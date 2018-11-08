left = MoveHubMotor(Port.A)
right = MoveHubMotor(Port.B)
base = DriveBase(left, right)
sensor = ColorDistanceSensor(Port.D)

while True:
    steering = ((sensor.reflection() - 45)*2)//3
    base.drive(60, steering)
