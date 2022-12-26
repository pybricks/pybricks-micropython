# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: Technic Hub (ble1_technic.py) and Prime Hub (ble1_prime.py)

Description: Tests broadcast receive and transmit.
"""

from pybricks.hubs import TechnicHub
from pybricks.parameters import Color
from pybricks.tools import wait
from pybricks.ble import Broadcast


hub = TechnicHub()
hub.light.on(Color.WHITE)

# Initialize broadcast
radio = Broadcast(signals=["number", "hue"])

for i in range(100):

    # Send one byte on the number signal.
    radio.transmit("number", bytes([i]))
    wait(100)

    # Receive two bytes on the hue signal.
    hue_data = radio.received("hue")

    # If we received something, set the light.
    if hue_data:
        hue = int.from_bytes(hue_data, "little")
        hub.light.on(Color(hue))
