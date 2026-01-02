from pybricks.hubs import VirtualHub

hub = VirtualHub(observe_channels=[123])

data = ()

while data != b"STOP":
    new = hub.ble.observe(123)
    if new == data:
        continue

    data = new
    print(data)
