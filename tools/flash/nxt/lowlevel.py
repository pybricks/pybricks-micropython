# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>

import time

import usb

USB_BULK_OUT_EP = 0x1
USB_BULK_IN_EP = 0x82


def enumerate_usb():
    """Return a generator yielding all attached USB devices."""
    busses = usb.busses()
    for bus in busses:
        devices = bus.devices
        for dev in devices:
            yield dev


def get_device(vendor_id, product_id, version=0, timeout=None):
    """Return the first device matching the given vendor/product ID."""
    while True:
        for dev in enumerate_usb():
            if (
                dev.idVendor == vendor_id
                and dev.idProduct == product_id
                and dev.iSerialNumber == version
            ):
                return UsbBrick(dev)
        if timeout is None or timeout <= 0:
            return None
        sleep_time = min(1.0, timeout)
        time.sleep(sleep_time)
        timeout -= sleep_time


class UsbBrick(object):
    def __init__(self, dev):
        self._dev = dev

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def open(self, interface, configuration=1, detach_kernel_driver=False):
        self._iface = interface
        self._config = configuration
        self._hdl = self._dev.open()
        if detach_kernel_driver:
            self._hdl.detachKernelDriver(interface)
        self._hdl.setConfiguration(configuration)
        self._hdl.claimInterface(interface)

    def close(self):
        self._hdl.releaseInterface()
        del self._hdl

    def read(self, size, timeout=100):
        """Read the given amount of data from the device and return it."""
        # For some reason, bulkRead returns a tuple of longs. This is
        # dumb, so we convert it back to a string before returning,
        # kthx.
        try:
            data = self._hdl.bulkRead(USB_BULK_IN_EP, size, timeout)
        except usb.USBError:
            return None
        return "".join(chr(x) for x in data)

    def write(self, data, timeout=100):
        """Write the given amount of data to the device."""
        return self._hdl.bulkWrite(USB_BULK_OUT_EP, data, timeout)
