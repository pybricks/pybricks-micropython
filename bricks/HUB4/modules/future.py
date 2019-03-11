class DriveBase:
    def __init__(self, left, right):
        self.left = left
        self.right = right
    
    def drive(self, speed, turn_rate):
        self.left.duty(speed + turn_rate)
        self.right.duty(speed - turn_rate)
