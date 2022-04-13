# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from enum import IntEnum
import ctypes


class PortId(IntEnum):
    """
    I/O port identifier.

    Values are the same as ``pbio_port_id_t``.
    """

    A = ord("A")
    """
    The port labeled "A".
    """
    B = ord("B")
    """
    The port labeled "B".
    """
    C = ord("C")
    """
    The port labeled "C".
    """
    D = ord("D")
    """
    The port labeled "D".
    """
    E = ord("E")
    """
    The port labeled "E".
    """
    F = ord("F")
    """
    The port labeled "F".
    """
    S1 = ord("1")
    """
    The port labeled "1".
    """
    S2 = ord("2")
    """
    The port labeled "2".
    """
    S3 = ord("3")
    """
    The port labeled "3".
    """
    S4 = ord("4")
    """
    The port labeled "4".
    """


class pbio_port_id_t(ctypes.c_uint32):
    pass


class pbio_iodev_data_type_t(ctypes.c_uint32):
    pass


class pbio_iodev_type_id_t(ctypes.c_uint32):
    pass


class pbio_iodev_capability_flags_t(ctypes.c_uint32):
    pass


class pbio_iodev_mode_t(ctypes.Structure):
    _fields_ = [
        ("num_values", ctypes.c_uint8),
        ("data_type", pbio_iodev_data_type_t),
    ]


class pbio_iodev_info_t(ctypes.Structure):
    _fields_ = [
        ("type_id", pbio_iodev_type_id_t),
        ("capability_flags", pbio_iodev_capability_flags_t),
        ("num_modes", ctypes.c_uint8),
        ("mode_combos", ctypes.c_uint16),
        ("mode_info", pbio_iodev_mode_t * 16),
    ]


class pbio_iodev_t(ctypes.Structure):
    """
    Data structure for holding an I/O device's state.
    """

    pass


set_mode_begin = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(pbio_iodev_t), ctypes.c_uint8)
set_mode_end = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(pbio_iodev_t))
set_mode_cancel = ctypes.CFUNCTYPE(None, ctypes.POINTER(pbio_iodev_t))

set_data_begin = ctypes.CFUNCTYPE(
    ctypes.c_uint32, ctypes.POINTER(pbio_iodev_t), ctypes.POINTER(ctypes.c_uint8)
)
set_data_end = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(pbio_iodev_t))
set_data_cancel = ctypes.CFUNCTYPE(None, ctypes.POINTER(pbio_iodev_t))

write_begin = ctypes.CFUNCTYPE(
    ctypes.c_uint32,
    ctypes.POINTER(pbio_iodev_t),
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_uint8,
)
write_end = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(pbio_iodev_t))
write_cancel = ctypes.CFUNCTYPE(None, ctypes.POINTER(pbio_iodev_t))


class pbio_iodev_ops_t(ctypes.Structure):
    _fields_ = [
        ("set_mode_begin", set_mode_begin),
        ("set_mode_end", set_mode_end),
        ("set_mode_cancel", set_mode_cancel),
        ("set_data_begin", set_data_begin),
        ("set_data_end", set_data_end),
        ("set_data_cancel", set_data_cancel),
        ("write_begin", write_begin),
        ("write_end", write_end),
        ("write_cancel", write_cancel),
    ]


pbio_iodev_t._fields_ = [
    ("info", ctypes.POINTER(pbio_iodev_info_t)),
    ("ops", ctypes.POINTER(pbio_iodev_ops_t)),
    ("port", pbio_port_id_t),
    ("mode", ctypes.c_uint8),
    # bin_data is 4-byte aligned so we have to add padding
    ("_pad", ctypes.c_uint8 * 3),
    ("bin_data", ctypes.c_uint8 * 32),
]


class VirtualIOPort:
    def __init__(self, port: PortId) -> None:
        """
        Creates a new virtual I/O port.

        Args:
            port: The port identifier.
        """
        self._info = pbio_iodev_info_t()
        self._ops = pbio_iodev_ops_t()
        self._iodev = pbio_iodev_t(
            info=ctypes.pointer(self._info),
            ops=ctypes.pointer(self._ops),
            port=port,
        )

    @property
    def iodev(self) -> int:
        """
        Gets the address of the ``pbio_iodev_t`` structure.

        This property is read when ``pbdrv_ioport_get_iodev()`` is called.
        """
        return ctypes.addressof(self._iodev)
