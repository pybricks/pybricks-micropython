from pybricks.hubs import VirtualHub
from pybricks.tools import wait

hub = VirtualHub(broadcast_channel=123)

for i in range(99):
    hub.ble.broadcast(["Hello", i])
    wait(100)

hub.ble.broadcast(b"STOP")
wait(500)
