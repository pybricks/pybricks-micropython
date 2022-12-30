# Copyright (c) 2022 The Pybricks Authors

"""
Hardware Module: Any hub (ble3_hub_2.py) and any other hub (ble3_hub_1.py)

Description: Tests broadcast bidirectional communication.
In particular the LED on the hub will indicate if there are data dropouts
"""

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import wait
from pybricks.experimental import Broadcast

# Initialize this hub.
hub = ThisHub()

# Initialize broadcast with two topics.
radio = Broadcast(topics=["1to2", "2to1"])

index = 0
last_data = 0
count_no_data = 0

while True:
    data = radio.receive("1to2")
    radio.send("2to1", index)

    if data:
        if last_data == data:
            count_no_data += 1
        else:
            last_data = data
            count_no_data = 0

        hub.light.on(Color((count_no_data * 10) % 360))

    else:
        hub.light.on(Color.WHITE)

    index += 1
    wait(100)
