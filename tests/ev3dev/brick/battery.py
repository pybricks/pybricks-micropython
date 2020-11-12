from pybricks.hubs import EV3Brick

ev3 = EV3Brick()

print(ev3.battery.voltage())  # 7400
print(ev3.battery.current())  # 180
print(ev3.battery.type())  # Alkaline
