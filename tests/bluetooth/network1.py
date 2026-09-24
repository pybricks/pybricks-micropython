# Prints what the brick running network0.py sends.

from pybricks.tools import wait
from pybricks.messaging import HubNetwork

radio = HubNetwork()
print("address", radio.address())

data = None

while data != "STOP":
    for address, data in radio.inbox():
        print("got", repr(data))
    wait(50)
