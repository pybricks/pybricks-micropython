# Broadcast and observe at once, on the hub under test.
#
# This is the case the Move Hub comment claims does not work: a hub that
# transmits and receives broadcasts at the same time. It measures the
# receiving half directly, with radio.trace(). The transmitting half is
# measured by scan_trace_count.py on a third device, since a hub cannot see
# its own advertisements.
#
# The first two windows only observe and the last two also broadcast, so the
# baseline and the case under test are measured back to back by one program.
# Nothing else changes between them, which is the whole point: comparing
# against a separate observe-only run would also be comparing two different
# radio environments.
#
# Run this twice, with no edits in between:
#
#   Run 1 (connected)    Start it from Pybricks Code and leave the hub
#                        connected for the whole run. Results are printed.
#   Run 2 (standalone)   Download it, disconnect, then start it with the hub
#                        button. Results are shown as a status light colour
#                        and afterwards broadcast for scan_trace_report.py.
#
# Hub B runs scan_trace_tx.py standalone throughout, on RX_CHANNEL. Its own
# rate never changes, so every difference seen here belongs to this hub.

from pybricks.hubs import ThisHub
from pybricks.messaging import BLERadio
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

# The reference transmitter, hub B.
RX_CHANNEL = 1

# Our own broadcasts, counted by scan_trace_count.py. The results are sent on
# the same channel once measuring is over, which is what scan_trace_report.py
# already listens on.
TX_CHANNEL = 2

WINDOWS = 4
BROADCAST_FROM = 2

WINDOW_MS = 15000
SHOW_MS = 3000

# Slower than the 100ms this hub advertises at, so every value goes out at
# least once and a receiver that misses nothing sees 1000 / TX_PERIOD_MS
# distinct values per second.
TX_PERIOD_MS = 50

# Must match BLE_TRACE_SIZE in pb_type_ble_radio.c, times four bytes a record.
# Allocated once, because trace() is called inside the measurement window and
# a garbage collection there would distort the gaps it is reporting.
BUF = bytearray(128 * 4)

# Upper edges of the gap histogram buckets, in units of 100 us, matching
# scan_trace_rx.py so the two can be compared.
EDGES = (150, 250, 400, 600, 900, 1500, 3000)


def bucket(rate_x10):
    if rate_x10 >= 100:
        return Color.GREEN
    if rate_x10 >= 50:
        return Color.CYAN
    if rate_x10 >= 20:
        return Color.ORANGE
    return Color.RED


hub = ThisHub()
radio = BLERadio(broadcast_channel=TX_CHANNEL, observe_channels=[RX_CHANNEL])

# Discard whatever accumulated before the program was ready, including the
# bogus first gap that is measured from the epoch.
radio.trace(BUF)

watch = StopWatch()
results = []
counter = 0

for index in range(WINDOWS):
    broadcasting = index >= BROADCAST_FROM
    hub.light.on(Color.MAGENTA if broadcasting else Color.YELLOW)

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

    radio.trace(BUF)
    watch.reset()
    next_ms = 0

    while watch.time() < WINDOW_MS:
        if broadcasting:
            # TEMPORARY: report which link layer command the chip refused
            # rather than stopping on the exception it raises.
            try:
                radio.broadcast(counter)
            except Exception:
                print("adv", radio.adv_status())
                raise
            counter = (counter + 1) % 32768

        # Absolute schedule, so the rate does not drift with the time
        # broadcast() itself takes.
        next_ms += TX_PERIOD_MS
        delay = next_ms - watch.time()
        if delay > 0:
            wait(delay)

        lost, size = radio.trace(BUF)
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
    print(index, broadcasting, rate_ours, rate_all, max_gap // 10, dropped, hist)
    wait(SHOW_MS)

# Broadcast the results forever so scan_trace_report.py can collect them after
# a standalone run. Measurement is over, so transmitting now is harmless.
hub.light.on(Color.WHITE)
while True:
    for index in range(len(results)):
        rate_ours, max_gap, dropped, hist = results[index]
        radio.broadcast((index, rate_ours, max_gap, hist))
        wait(700)
