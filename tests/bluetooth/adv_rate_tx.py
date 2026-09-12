# Advertising interval A/B, transmitter. Broadcast only, nothing else.
#
# The advertising interval used to be whatever the GAP layer picked, and is now
# ours to choose, so this checks that the new choice is not slower than the old
# one. It broadcasts and never observes, so the interval is the only thing that
# decides how often a value goes out.
#
# Run it standalone on the hub under test, once per firmware being compared,
# with scan_trace_count.py counting on a spare hub. The spare hub's own
# reception is the same in both runs, so the two counts can be compared even
# though neither is the transmit rate itself.
#
# Works on released firmware: no instrumentation and no bytearray, so it runs
# on a Move Hub built from master as well as from the branch under test.

from pybricks.hubs import ThisHub
from pybricks.messaging import BLERadio
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

# Matches TX_CHANNEL in scan_trace_count.py.
CHANNEL = 2

# Faster than any advertising interval the chip can use, so the value that goes
# out is always a fresh one and the count is limited by the interval alone.
TX_PERIOD_MS = 50

hub = ThisHub()
radio = BLERadio(broadcast_channel=CHANNEL)
hub.light.on(Color.BLUE)

watch = StopWatch()
counter = 0
next_ms = 0

while True:
    radio.broadcast(counter)
    counter = (counter + 1) % 32768

    # Absolute schedule, so the rate does not drift with the time broadcast()
    # itself takes.
    next_ms += TX_PERIOD_MS
    delay = next_ms - watch.time()
    if delay > 0:
        wait(delay)
