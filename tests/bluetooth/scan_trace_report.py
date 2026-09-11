# Go/no-go test, report reader, only needed after a standalone run.
#
# Once hub A has finished measuring it broadcasts its results forever, so stop
# scan_trace_tx.py on hub B at that point and run this instead, from Pybricks Code.
# Hub A's measurement is already over by then, so nothing this hub does can
# affect the numbers.

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import wait

REPORT_CHANNEL = 2
WINDOWS = 4

hub = ThisHub(observe_channels=[REPORT_CHANNEL])
hub.light.on(Color.WHITE)

seen = [None] * WINDOWS
count = 0

# Rates are per second times ten. The histogram columns are gaps between
# arrivals in ms: under 15, 15-25, 25-40, 40-60, 60-90, 90-150, 150-300, over
# 300.
print("window ours all max_gap_ms histogram")

while count < WINDOWS:
    data = hub.ble.observe(REPORT_CHANNEL)
    if data is not None:
        index, rate_ours, rate_all, max_gap, hist = data
        if 0 <= index < WINDOWS and seen[index] is None:
            seen[index] = data
            count += 1
            print(index, rate_ours, rate_all, max_gap, list(hist))
    wait(50)

hub.light.on(Color.GREEN)
