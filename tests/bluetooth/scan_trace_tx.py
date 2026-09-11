# Go/no-go test 1 of 3: the transmitter (hub B).
#
# Broadcasts a counter and does nothing else. In particular it never observes,
# so this hub's transmit rate is the undisturbed ~28-33 packets per second and
# is identical in both runs of the test.
#
# Start it with the hub button and leave it standalone for the whole test. If
# it is connected to Pybricks Code its transmit rate drops and the measurement
# on hub A no longer isolates hub A's receiving.

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

CHANNEL = 1

# The hub advertises every 25-35 ms (TGAP_GEN_DISC_ADV_INT 40 plus the 0-10 ms
# random advDelay the Bluetooth spec requires). Stepping the counter slower
# than that means every value goes out at least once, so a receiver that
# misses nothing sees 1000 / TX_PERIOD_MS distinct values per second.
TX_PERIOD_MS = 50

# Keeps the payload at 3 bytes and keeps wrap-around easy to undo.
MODULUS = 32768

hub = ThisHub(broadcast_channel=CHANNEL)
hub.light.on(Color.BLUE)

watch = StopWatch()
counter = 0
next_ms = 0

while True:
    hub.ble.broadcast(counter)
    counter = (counter + 1) % MODULUS

    # Absolute schedule, so the rate does not drift with the time broadcast()
    # itself takes.
    next_ms += TX_PERIOD_MS
    delay = next_ms - watch.time()
    if delay > 0:
        wait(delay)
