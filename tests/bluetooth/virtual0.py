from pybricks.tools import wait
from pybricks.messaging import BLERadio

radio = BLERadio(broadcast_channel=123)

for i in range(99):
    radio.broadcast(["Hello", i])
    wait(100)

radio.broadcast(b"STOP")
wait(500)
