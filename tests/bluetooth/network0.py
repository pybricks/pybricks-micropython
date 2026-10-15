# Sends every kind of message that HubNetwork can carry, and then the ones it
# cannot. The brick running network1.py prints what arrives, which should be
# the same as what this prints, in the same order.

from pybricks.tools import wait
from pybricks.messaging import HubNetwork

# Address of the brick running network1.py, which it prints when it starts.
OTHER = "CC:BA:BD:6A:14:45"

radio = HubNetwork()
print(radio.address())
radio.connect(OTHER)

for data in (
    b"raw bytes",
    "hello",
    3.25,
    42,
    -7,
    True,
    False,
    None,
    ("hello", 3.25),
    ("a", 1, 2.5, True, None, b"xy"),
    ("only",),
    (),
    "",
    b"",
    "pi π ok",
    b"\x00\xff",
    (b"\x00\xff", "\x00end"),
    tuple(range(32)),
    b"x" * 255,
):
    print("sent", repr(data))
    radio.send(data, OTHER)
    wait(250)

for data in (
    [1, 2],
    (1, [2]),
    tuple(range(33)),
    b"x" * 256,
    "y" * 300,
    ("z" * 200, "z" * 100),
    (2**40,),
):
    try:
        radio.send(data, OTHER)
        print("not refused", repr(data)[:24])
    except Exception as e:
        print("refused", repr(data)[:24], repr(e))

radio.send("STOP", OTHER)
