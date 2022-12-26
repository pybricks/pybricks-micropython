# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: Technic Hub (ble1_technic.py) and Prime Hub (ble1_prime.py)

Description: Tests broadcast receive and transmit.
"""

from pybricks.hubs import PrimeHub
from pybricks.parameters import Color
from pybricks.tools import wait
from pybricks.ble import Broadcast


hub = PrimeHub()
hub.light.on(Color.WHITE)

# Initialize broadcast with two topics.
radio = Broadcast(topics=["counter", "hue"])

while True:

    # Read the counter value.
    counter_data = radio.receive_bytes("counter")

    # If we received it, display it, and send a response.
    if counter_data:
        count = counter_data[0]
        hub.display.number(count)

        # Send back a hue value.
        hue_data = int(count * 3.6).to_bytes(2, "little")
        radio.send_bytes("hue", hue_data)

        # Stop if we reach the end.
        if count >= 99:
            break

    # Don't need to update all the time.
    wait(10)
