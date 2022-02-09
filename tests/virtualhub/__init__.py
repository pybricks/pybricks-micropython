# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


class VirtualHub:
    """
    Base class for virtual hub implementations.
    """

    def on_event_poll(self) -> None:
        """
        This method is called during MICROPY_EVENT_POLL_HOOK in MicroPython.

        The default implementation does nothing, so overriding methods don't
        need to call ``super().on_event_poll()``.
        """
        pass
