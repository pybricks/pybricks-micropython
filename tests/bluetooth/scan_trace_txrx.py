# Broadcast throughput test, hub A. Run this connected to Pybricks Code.
#
# The scan tuning only applies while the hub is actually scanning, so a hub
# that only broadcasts cannot show what the tuning costs it. This one does both
# at once, which is the case that matters: a hub talking to Pybricks Code while
# broadcasting to and observing other hubs.
#
# It observes a channel nobody transmits on. That still makes the Bluetooth
# chip scan, which is the point, and it avoids calling observe(), which would
# ask for an observing restart every time reception paused for a second.
#
# Hub B runs scan_trace_rx.py standalone and counts what arrives here on
# CHANNEL. Hub B's own scan duty is unchanged by any of this, so its count is
# proportional to how often this hub actually got an advertisement out.

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import StopWatch, wait

CHANNEL = 1

# A channel nothing transmits on, purely to keep the radio scanning.
IDLE_CHANNEL = 3

TX_PERIOD_MS = 50
REPORT_MS = 15000

# Must match BLE_TRACE_SIZE in pb_type_ble_radio.c, times four bytes a record.
BUF = bytearray(128 * 4)

hub = ThisHub(broadcast_channel=CHANNEL, observe_channels=[IDLE_CHANNEL])
hub.light.on(Color.MAGENTA)

watch = StopWatch()
report = StopWatch()
counter = 0
next_ms = 0
heard = 0
dropped = 0

hub.ble.trace(BUF)

while True:
    hub.ble.broadcast(counter)
    counter = (counter + 1) % 32768

    next_ms += TX_PERIOD_MS
    delay = next_ms - watch.time()
    if delay > 0:
        wait(delay)

    # How much this hub is receiving while doing both jobs at once. Everything
    # here is foreign traffic, since nothing transmits on IDLE_CHANNEL, but the
    # count still says how much of the time the radio was listening.
    lost, size = hub.ble.trace(BUF)
    dropped += lost
    heard += size // 4

    if report.time() >= REPORT_MS:
        elapsed = report.time()
        print(counter, heard * 10000 // elapsed, dropped, elapsed)
        heard = 0
        dropped = 0
        report.reset()
