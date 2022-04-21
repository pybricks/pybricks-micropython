# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

import random
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

    @property
    def microseconds(self) -> int:
        """
        The current clock time in microseconds.
        """
        return self.nanoseconds // 1000


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


class CountingClock(VirtualClock):
    """
    Clock implementation that increases the current time by *step* microseconds
    each time :meth:`tick` is called.

    To create a clock that runs as fast as possible use the class like this::

        class Platform(VirtualPlatform):
            def __init__(self):
                super().__init__()
                ...
                self.clock[-1] = CountingClock()
                self.subscribe_poll(self.clock[-1].tick)
                ...
    """

    def __init__(
        self, start: int = (2**32 - 3) * 1000, step: int = 1000, fuzz: int = 100
    ) -> None:
        """
        Args:
            start:
                The starting time in microseconds. The default value is chosen
                so that the 32-bit millisecond clock and the 32-bit microsecond
                clock will both roll over in two virtual seconds. This is done
                to help expose code that may not be computing time differences
                correctly.
            step:
                The number of microseconds to increase the clock time by on
                each :meth:`tick`. The default value advances the virtual clock
                by 1 millisecond on each tick.
            fuzz:
                The amount of random variation to apply to *step* at each tick.
                This is done to help expose code that may not correctly handle
                small variations in timestamps. Setting to 0 will disable
                fuzzing.
        """
        super().__init__()
        # convert microseconds to nanoseconds
        self.nanoseconds = start * 1000
        self._step_ns = step * 1000
        self._fuzz_ns = fuzz * 1000

    def tick(self, *args):
        """
        Increases the clock time by *step* +/- *fuzz* and triggers the clock interupt.

        This method has unused *args so that it can be passed directly to the
        :class:`VirtualPlatform` poll subscribe method.
        """
        if self._fuzz_ns:
            self.nanoseconds += random.randint(
                self._step_ns - self._fuzz_ns, self._step_ns + self._fuzz_ns
            )
        else:
            self.nanoseconds += self._step_ns

        self.interrupt()
