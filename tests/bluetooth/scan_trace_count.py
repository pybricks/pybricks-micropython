# The transmitting half of the broadcast and observe test, on a third device.
#
# A hub cannot see its own advertisements, so this counts how many of the hub
# under test's broadcasts actually reach anyone. It reports distinct values per
# second, which is a lower bound on the transmit rate: the hub under test steps
# its counter every TX_PERIOD_MS there, so a device that misses nothing sees
# 1000 / TX_PERIOD_MS of them.
#
# Run it from Pybricks Code on a spare hub and start it before the hub under
# test, then read the rate during each of that hub's windows. The first two
# windows are silent by design, so the interesting number is the difference
# between the last two and what a hub that only broadcasts manages.
#
# This hub does nothing but observe, so it does not compete for the hub under
# test's radio. It is a passive listener and does not transmit at all.

from pybricks.hubs import ThisHub
from pybricks.messaging import BLERadio
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

# The hub under test's broadcasts, matching TX_CHANNEL in scan_trace_both.py.
TX_CHANNEL = 2

REPORT_MS = 5000
POLL_MS = 10

hub = ThisHub()
radio = BLERadio(observe_channels=[TX_CHANNEL])
hub.light.on(Color.CYAN)

report = StopWatch()
seen = 0
last = None

print("rate*10 distinct elapsed_ms")

while True:
    data = radio.observe(TX_CHANNEL)

    # Only the plain counter is of interest. Once measuring is over the hub
    # under test sends its results as a tuple on the same channel, which is
    # where the rate drops to nothing and the run is done.
    if isinstance(data, int) and data != last:
        last = data
        seen += 1

    wait(POLL_MS)

    if report.time() >= REPORT_MS:
        elapsed = report.time()
        print(seen * 10000 // elapsed, seen, elapsed)
        seen = 0
        report.reset()
