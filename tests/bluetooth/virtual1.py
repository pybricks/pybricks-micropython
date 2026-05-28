from pybricks.messaging import BLERadio

radio = BLERadio(observe_channels=[123])

data = ()

while data != b"STOP":
    new = radio.observe(123)
    if new == data:
        continue

    data = new
    print(data)
