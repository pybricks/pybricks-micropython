from pybricks.hubs import EV3Brick

ev3 = EV3Brick()

print(ev3.battery.voltage())  # 7400
print(ev3.battery.current())  # 180
print(ev3.battery.type())  # Alkaline
try:
    ev3.battery.temperature()
except OSError as ex:
    print(ex.args[0])  # 95: EOPNOTSUPP
