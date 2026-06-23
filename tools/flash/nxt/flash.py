# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>

import math
from importlib.resources import read_binary

from nxt import resources

from .firmware import Firmware
from .samba import SambaBrick, SambaOpenError


import sys

# Mnemonics for the addresses of the various registers used by the flash
# controller.
PMC_MCKR = 0xFFFFFC30  # Master clock register
MC_FMR = 0xFFFFFF60  # Flash mode register
MC_FCR = 0xFFFFFF64  # Flash control register
MC_FSR = 0xFFFFFF68  # Flash status register

# The addresses in ram used by the flash driver.
FLASH_DRIVER_ADDR = 0x202000
FLASH_TARGET_BLOCK_NUM_ADDR = 0x202300
FLASH_BLOCK_DATA_ADDR = 0x202100

# The region lock control bits are non-volatile bits, which have special
# timing requirements compared to the 'regular' flash memory. The
# following constants define the timing settings for lock bits and for
# normal flash memory.
#
# FMCN = 0x5, FWS = 0x1
FLASH_REGION_LOCK_SETTING = (0x5 << 16) | (0x1 << 8)
# FMCN = 0x34, FWS = 0x1
FLASH_MEMORY_WRITE_SETTING = (0x34 << 16) | (0x1 << 8)

# The base command to unlock a flash region, and a helper to generate the
# correct region number.
FLASH_REGION_UNLOCK_CMD = (0x5A << 24) | (0x4)


def _unlock_region(region_num):
    # The unlock command must specify the page number of any page within
    # the region that you want to unlock. Since each region is 64 pages
    # long, we just multiply the page number by 64 to get into the
    # correct region.
    return FLASH_REGION_UNLOCK_CMD | ((64 * region_num) << 8)


class MissingFlashDriverFile(Exception):
    """Could not find the flash driver firmware image."""


class InvalidFirmwareImage(Exception):
    """The given firmware image cannot be written."""


class FlashController(object):
    def __init__(self, brick):
        self._brick = brick

    def _wait_for_flash(self):
        """Wait for the flash controller to become ready."""
        status = self._brick.read_word(MC_FSR)
        while not (status & 0x1):
            status = self._brick.read_word(MC_FSR)

    def _unlock_regions(self):
        status = self._brick.read_word(MC_FSR)
        if (status & 0xFFFF0000) == 0:
            return

        self._brick.write_word(MC_FMR, FLASH_REGION_LOCK_SETTING)
        for i in range(16):
            mask = 1 << (16 + i)
            if status & mask:
                self._brick.write_word(MC_FCR, _unlock_region(i))
                self._wait_for_flash()
        self._brick.write_word(MC_FMR, FLASH_MEMORY_WRITE_SETTING)

    def _prepare_flash(self):
        # Switch to the PLL clock with a /2 prescaler. The timing
        # configurations we use are only valid for that configuration.
        #
        # TODO: The atmel tool does this as well, but according to the
        # specs this is horribly wrong: one should absolutely not switch
        # clock sources and prescalers in one go, much less switch to
        # the PLL without configuring it and stabilizing it first. What
        # the hell is this?
        self._brick.write_word(PMC_MCKR, 0x7)

        # Set the correct timings for flash writes.
        self._brick.write_word(MC_FMR, FLASH_MEMORY_WRITE_SETTING)

        # Check/unlock all flash regions.
        self._unlock_regions()

        # Send the flash driver to the brick.
        driver = read_binary(resources, resources.FLASH_DRIVER)
        self._brick.write_buffer(FLASH_DRIVER_ADDR, driver)

    def flash(self, firmware):
        self._prepare_flash()

        num_pages = int(math.ceil(len(firmware) / 256))
        if num_pages > 1024:
            raise InvalidFirmwareImage("The firmware image must be smaller than 256kB")

        for page_num in range(num_pages):
            self._brick.write_word(FLASH_TARGET_BLOCK_NUM_ADDR, page_num)
            self._brick.write_buffer(
                FLASH_BLOCK_DATA_ADDR, firmware[page_num * 256 : (page_num + 1) * 256]
            )
            self._brick.jump(FLASH_DRIVER_ADDR)


def flash_nxt(firmwares: dict[str, bytes]) -> None:
    """
    Flashes firmware to NXT using the Samba bootloader.

    Args:
        firmware:
            A firmware blob with the NxOS header appended to the end.
    """

    # There is only one firmware for the NXT.
    firmware = firmwares["nxt"]

    # parse the header
    info = Firmware(firmware)

    if info.samba:
        raise ValueError("Firmware is not suitable for flashing.")

    s = SambaBrick()

    try:
        print("Looking for the NXT in SAM-BA mode...")
        s.open(timeout=5)
        print("Brick found!")
    except SambaOpenError as e:
        print(e)
        sys.exit(1)

    print("Flashing firmware...")
    f = FlashController(s)
    f.flash(firmware)

    print("Flashing complete, jumping to 0x100000...")
    f._wait_for_flash()
    s.jump(0x100000)

    print("Firmware started.")
    s.close()
