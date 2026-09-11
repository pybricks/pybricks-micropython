# Go/no-go test, observer (hub A), using the hub.ble.trace() instrumentation
# from the ble-scan-trace branch. Released firmware cannot report reception
# at all, so without that instrumentation this has to be inferred from how
# many distinct broadcast values arrive, which is only a lower bound.
#
# Run this twice, with no edits in between:
#
#   Run 1 (connected)    Start it from Pybricks Code and leave the hub
#                        connected for the whole run. Results are printed.
#   Run 2 (standalone)   Download it, disconnect, then start it with the hub
#                        button. Results are shown as a status light colour
#                        and afterwards broadcast for scan_trace_report.py.
#
# This hub transmits nothing while it is measuring: no broadcast data is set
# yet, and the system stops advertising for a connection as soon as a user
# program starts. Nor does it call observe(), which would trip the observing
# restart workaround for support#1096 whenever reception paused for a second.
# So the only difference between the two runs is the connection.
#
# The gaps between arrivals are the real prize. The chip scans for a window
# and then goes quiet for the rest of the scan period, so arrivals bunch up
# inside the windows. A histogram of the gaps therefore shows the scan
# schedule the chip is actually running, which is what the connected case is
# suspected of cutting short.

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

RX_CHANNEL = 1
REPORT_CHANNEL = 2

WINDOWS = 4
WINDOW_MS = 15000
SHOW_MS = 3000
POLL_MS = 500

# Must match BLE_TRACE_SIZE in pb_type_ble_radio.c, times four bytes a record.
# Allocated once, because trace() is called inside the measurement window and
# a garbage collection there would distort the gaps it is reporting.
BUF = bytearray(128 * 4)

# Upper edges of the gap histogram buckets, in units of 100 us. Chosen around
# the 25-35 ms advertising interval and the 70 ms scan period: gaps inside a
# scan window land in the low buckets, gaps that span one or more scan periods
# land in the high ones.
EDGES = (150, 250, 400, 600, 900, 1500, 3000)


def bucket(rate_x10):
    # The prediction under test is green when standalone, red or orange when
    # connected.
    if rate_x10 >= 100:
        return Color.GREEN
    if rate_x10 >= 50:
        return Color.CYAN
    if rate_x10 >= 20:
        return Color.ORANGE
    return Color.RED


hub = ThisHub(broadcast_channel=REPORT_CHANNEL, observe_channels=[RX_CHANNEL])

# Discard whatever accumulated before the program was ready, including the
# bogus first gap that is measured from the epoch.
hub.ble.trace(BUF)

watch = StopWatch()
results = []

for index in range(WINDOWS):
    hub.light.on(Color.YELLOW)

    hist = [0] * (len(EDGES) + 1)
    ours = 0
    total = 0
    max_gap = 0
    dropped = 0

    # Gaps have to be accumulated across advertisements from other devices.
    # Binning the raw gap would measure how often the radio heard anything at
    # all, which depends on whatever else is transmitting nearby, rather than
    # how long this hub waited for an update from the hub under test.
    gap = 0

    hub.ble.trace(BUF)
    watch.reset()

    while watch.time() < WINDOW_MS:
        wait(POLL_MS)
        lost, size = hub.ble.trace(BUF)
        dropped += lost

        for i in range(0, size, 4):
            delta = BUF[i] | (BUF[i + 1] << 8)
            channel = BUF[i + 3]

            total += 1
            gap += delta

            if channel != RX_CHANNEL:
                continue

            ours += 1
            if gap > max_gap:
                max_gap = gap

            slot = len(EDGES)
            for edge in range(len(EDGES)):
                if gap < EDGES[edge]:
                    slot = edge
                    break
            hist[slot] += 1
            gap = 0

    elapsed = watch.time()

    rate_ours = ours * 10000 // elapsed
    rate_all = total * 10000 // elapsed

    results.append(
        (
            rate_ours,
            max_gap // 10,
            dropped,
            bytes([h if h < 255 else 255 for h in hist]),
        )
    )

    # Visible when standalone. Printing is deliberately left until the window
    # is closed, so the stdio traffic it generates in the connected run cannot
    # disturb the measurement.
    hub.light.on(bucket(rate_ours))
    print(index, rate_ours, rate_all, max_gap // 10, dropped, hist)
    wait(SHOW_MS)

# Broadcast the results forever so scan_trace_report.py can collect them after a
# standalone run. Measurement is over, so transmitting now is harmless.
hub.light.on(Color.WHITE)
while True:
    for index in range(len(results)):
        rate_ours, max_gap, dropped, hist = results[index]
        hub.ble.broadcast((index, rate_ours, max_gap, hist))
        wait(700)
