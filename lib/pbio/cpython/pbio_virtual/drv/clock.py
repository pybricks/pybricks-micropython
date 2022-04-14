# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

import signal
import threading
import time


class VirtualClock:
    nanoseconds: int = 0
    """
    The current clock time in nanoseconds.

    This value is read when ``pbdrv_clock_get_ms()`` or ``pbdrv_clock_get_us``
    is called.
    """

    _thread_id: int
    _signum: int

    def interrupt(self):
        """
        Fires the clock "interrupt".

        This must not be called before :meth:`on_init`.
        """
        signal.pthread_kill(self._thread_id, self._signum)

    def on_init(self, thread_id: int, signum: int):
        """
        Called when ``pbdrv_clock_init()`` is called.

        Args:
            thread_id: The id of the thread to be interrupted.
            signum: The signal number to use when interrupting.
        """
        self._thread_id = thread_id
        self._signum = signum


class WallClock(VirtualClock):
    """
    Implementation of the virtual clock that uses the computer's own clock.
    """

    @property
    def nanoseconds(self) -> int:
        return time.monotonic_ns()

    def on_init(self, thread_id: int, signum: int):
        super().on_init(thread_id, signum)
        threading.Thread(target=self.run, daemon=True).start()

    def run(self):
        # Provide the required 1 millisecond tick in "real" time. In reality,
        # this won't be accurate since desktop OSes are not realtime systems.
        while True:
            time.sleep(0.001)
            self.interrupt()
