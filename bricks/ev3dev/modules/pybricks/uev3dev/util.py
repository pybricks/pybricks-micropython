# SPDX-License-Identifier: MIT
# Copyright (c) 2017 David Lechner <david@lechnology.com>

"""Utility module"""

import ffi
import _thread
import os
import sys
import utime

from uerrno import EINTR

from uselect import poll
from uselect import POLLIN
from uctypes import addressof
from uctypes import sizeof
from uctypes import struct
from uctypes import UINT32
from uctypes import UINT64

_libc = ffi.open('libc.so.6')

_eventfd = _libc.func('i', 'eventfd', 'ii')
_errno = _libc.var('i', 'errno')
_EFD_CLOEXEC = 0o2000000


def debug_print(*args, sep=' ', end='\n'):
    """Print on stderr for debugging

    Parameters:
        args: One or more arguments to print
        sep (str): Separator inserted between arguments
        end (str): Appended after the last argument
    """
    print(*args, sep=sep, end=end, file=sys.stderr)


def _thread_runner(lock, function):
    if not lock.locked():
        raise RuntimeError('Must hold lock before starting thread')
    try:
        function()
    finally:
        lock.release()


def fork(*functions):
    """Run functions in separate threads.

    This does not return until all threads have ended.

    Parameters:
        functions: one or more functions to run
    """
    locks = []
    for f in functions:
        lock = _thread.allocate_lock()
        lock.acquire()
        _thread.start_new_thread(_thread_runner, (lock, f))
        locks.append(lock)
    for l in locks:
        l.acquire()
        l.release()


class Timer():
    """Object that represents a timer"""
    def __init__(self):
        self._start_time = 0
        self.reset()

    def reset(self):
        self._start_time = utime.ticks_ms()

    def elapsed_time(self):
        now = utime.ticks_ms()
        return (now - self._start_time) / 1000

    def wait(self, compare, value):
        while not compare(self.elapsed_time(), value):
            pass


def write_at_index(array, index, value):
    """Creates a copy of an array with the value at ``index`` set to
    ``value``. The array is extended if needed.

    Parameters:
        array (tuple): The input array
        index (int): The index in the array to write to
        value (value): The value to write

    Returns:
        A new tuple containing the modified array
    """
    l = list(array)
    extend = index - len(l) + 1
    if extend > 0:
        # FIXME: this could be a logic array, in which case we should use
        # False instead of 0. But, using zero is not likely to cause problems.
        l += [0] * extend
    l[index] = value
    return tuple(l)


class Timeout():
    """Object for scheduling a callback.

    This is a pseudo replacement for ``threading.Timer``.

    .. warning:: Calling :py:meth:`start`, :py:meth:`cancel` or :py:meth:`wait`
        from the callback will cause a deadlock.

    Parameters:
        interval (float): The delay in seconds
        func (function): The callback function
        repeat (bool): When ``True``, the callback will be be called at
            ``interval`` until :py:meth:`cancel` is called. When ``False``,
            the callback will only be called once.
    """

    _ONE = int(1).to_bytes(8, sys.byteorder)

    def __init__(self, interval, func, repeat=False):
        self._fd = _eventfd(0, _EFD_CLOEXEC)
        if self._fd == -1:
            raise OSError(_errno.get())
        self._poll = poll()
        self._poll.register(self._fd, POLLIN)
        self._interval = interval
        self._func = func
        self._repeat = repeat
        # _cancel_lock protects access to _canceled
        self._cancel_lock = _thread.allocate_lock()
        self._canceled = False
        # _wait_lock is used for synchronization
        self._wait_lock = _thread.allocate_lock()

    def close(self):
        """Release operating system resources.

        Since micropython doesn't allow ``__del__`` on user-defined classes, we
        need to be sure to always manually call ``close()``.
        """
        self._poll.unregister(self._timerfd)
        self._poll.unregister(self._fd)
        self._poll.close()
        os.close(self._timerfd)
        os.close(self._fd)

    def _run(self):
        try:
            data = bytearray(8)
            while True:
                try:
                    events = self._poll.poll(int(self._interval * 1000))
                except OSError as err:
                    # TODO: implement PEP 475 in micropython so we don't have
                    # to check for EINTR
                    if err.args[0] == EINTR:
                        continue
                    raise
                for fd, ev in events:
                    e = os.read_(fd, data, 8)
                    os.check_error(e)
                with self._cancel_lock:
                    if self._canceled:
                        break
                    self._canceled = True
                    self._func()
                    if not self._repeat:
                        break
        finally:
            self._wait_lock.release()

    def start(self):
        """Start running the timer"""
        with self._cancel_lock:
            if self._wait_lock.locked():
                raise RuntimeError('already started')
            self._canceled = False
            self._wait_lock.acquire()
            _thread.start_new_thread(self._run, ())

    def cancel(self):
        """Cancel the timer"""
        with self._cancel_lock:
            if self._canceled:
                return
            self._canceled = True

            e = os.write(self._fd, Timeout._ONE)
            os.check_error(e)

    def wait(self):
        """Waits until the timer has finished or is canceled"""
        with self._wait_lock:
            pass
