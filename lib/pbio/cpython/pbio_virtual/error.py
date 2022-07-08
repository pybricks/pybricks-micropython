# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors


from enum import IntEnum


class PbioErrorCode(IntEnum):
    """
    PBIO error codes.

    These values match ``pbio_error_t``.
    """

    SUCCESS = 0
    """
    No error
    """
    FAILED = 1
    """
    Unspecified error (used when no other error code fits)
    """
    INVALID_ARG = 2
    """
    Invalid argument
    """
    IO = 3
    """
    General I/O error
    """
    BUSY = 4
    """
    Device or resource is busy
    """
    NO_DEV = 5
    """
    Device is not connected
    """
    NOT_IMPLEMENTED = 6
    """
    Feature is not yet implemented
    """
    NOT_SUPPORTED = 7
    """
    Feature is not supported on this device
    """
    AGAIN = 8
    """
    Function should be called again later
    """
    INVALID_OP = 9
    """
    Operation is not permitted in the current state
    """
    TIMEDOUT = 10
    """
    The operation has timed out
    """
    CANCELED = 11
    """
    The operation was canceled
    """


class PbioError(Exception):
    """
    Exception that wraps a :class:`PbioErrorCode`.

    Callbacks from PBIO into CPython will catch these errors and return
    *pbio_error*.

    Args:
        error: The error code.
    """

    def __init__(self, error: PbioErrorCode):
        if not isinstance(error, PbioErrorCode):
            raise TypeError

        super().__init__(error)

    @property
    def pbio_error(self) -> PbioErrorCode:
        """
        Gets the PBIO error code.
        """
        return self.args[0]
