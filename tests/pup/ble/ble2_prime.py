# Copyright (c) 2021 The Pybricks Authors

"""
Hardware Module: Any hub (ble2_technic.py) and any other hub (ble2_prime.py)

Description: Tests broadcast receive and transmit.
"""


from pybricks.parameters import Color
from pybricks.tools import wait
from pybricks.ble import Broadcast

from umath import pi


# Initialize broadcast with two topics.
radio = Broadcast(topics=["data"])

# This hub only sends, so we can turn off scanning.
radio.scan(False)

for i in range(20):

    # Send a number
    radio.send("data", 123)
    wait(1000)

    # Send a large number and a float
    radio.send("data", (-456789, pi))
    wait(1000)

    # Send even more.
    radio.send("data", ("or", 1.234, "this!"))
    wait(1000)

    # Send nothing, to clear their data.
    radio.send("data", None)
    wait(1000)
