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

# Initialize broadcast
radio = Broadcast(signals=["number", "hue"])

while True:

    # Receive one number.
    number_data = radio.received("number")

    # If we received a number, display it, and send a response.
    if number_data:
        number = number_data[0]
        hub.display.number(number)

        # Send back a hue value.
        hue_data = int(number * 3.6).to_bytes(2, "little")
        radio.transmit("hue", hue_data)

        # Stop if we reach the end.
        if number >= 99:
            break

    # Don't need to update all the time.
    wait(10)
