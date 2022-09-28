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


class IODeviceTypeId(IntEnum):
    """
    I/O device type identifiers.

    Values are the same as ``pbio_iodev_type_id_t``.
    """

    NONE = 0
    """
    No device is present
    """

    # LEGO Powered Up non-UART devices - some of these don't exist in real-life
    LPF2_MMOTOR = 1
    """
    45303 Powered Up Medium Motor (aka WeDo 2.0 motor)
    """
    LPF2_TRAIN = 2
    """
    Powered Up Train Motor
    """
    LPF2_TURN = 3
    LPF2_POWER = 4
    LPF2_TOUCH = 5
    LPF2_LMOTOR = 6
    LPF2_XMOTOR = 7
    LPF2_LIGHT = 8
    """
    88005 Powered Up Lights
    """
    LPF2_LIGHT1 = 9
    LPF2_LIGHT2 = 10
    LPF2_TPOINT = 11
    LPF2_EXPLOD = 12
    LPF2_3_PART = 13
    LPF2_UNKNOWN_UART = 14
    """
    Temporary ID for UART devices until real ID is read from the device.
    """

    # LEGO EV3 UART devices

    EV3_COLOR_SENSOR = 29
    """
    MINDSTORMS EV3 Color Sensor
    """

    EV3_ULTRASONIC_SENSOR = 30
    """
    MINDSTORMS EV3 Ultrasonic Sensor
    """

    EV3_GYRO_SENSOR = 32
    """
    MINDSTORMS EV3 Gyro Sensor
    """

    EV3_IR_SENSOR = 33
    """
    MINDSTORMS EV3 Infrared Sensor
    """

    # WeDo 2.0 UART devices

    WEDO2_TILT_SENSOR = 34
    """
    WeDo 2.0 Tilt Sensor
    """

    WEDO2_MOTION_SENSOR = 35
    """
    WeDo 2.0 Motion Sensor
    """

    WEDO2_GENERIC_SENSOR = 36

    # BOOST UART devices and motors

    COLOR_DIST_SENSOR = 37
    """
    BOOST Color and Distance Sensor
    """

    INTERACTIVE_MOTOR = 38
    """
    BOOST Interactive Motor
    """

    MOVE_HUB_MOTOR = 39
    """
    BOOST Move Hub built-in Motor
    """

    # Technic motors

    TECHNIC_L_MOTOR = 46
    """
    Technic Large Motor
    """

    TECHNIC_XL_MOTOR = 47
    """
    Technic XL Motor
    """

    # SPIKE motors

    SPIKE_M_MOTOR = 48
    """
    SPIKE Medium Motor
    """

    SPIKE_L_MOTOR = 49
    """
    SPIKE Large Motor
    """

    # SPIKE sensors

    SPIKE_COLOR_SENSOR = 61
    """
    SPIKE Color Sensor
    """

    SPIKE_ULTRASONIC_SENSOR = 62
    """
    SPIKE Ultrasonic Sensor
    """

    SPIKE_FORCE_SENSOR = 63
    """
    SPIKE Prime Force Sensor
    """

    TECHNIC_COLOR_LIGHT_MATRIX = 64
    """
    Technic Color Light Matrix
    """

    SPIKE_S_MOTOR = 65
    """
    SPIKE Small Motor
    """

    # Technic Angular Motors
    TECHNIC_M_ANGULAR_MOTOR = 75

    TECHNIC_L_ANGULAR_MOTOR = 76


class IODeviceCapabilityFlags(IntEnum):
    """
    I/O device device capability flags.

    Values are the same as ``pbio_iodev_capability_flags_t``.
    """

    PBIO_IODEV_CAPABILITY_FLAG_NONE = 0
    """
    Convenience value for no flags set.
    """

    PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT = 1 << 0
    """
    Indicates that this device is a DC output such as a motor or a light.
    """

    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_SPEED = 1 << 1
    """
    Indicates that the motor provides speed feedback.
    """

    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS = 1 << 2
    """
    Indicates that the motor provides relative position feedback.
    """

    PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS = 1 << 3
    """
    Indicates that the motor provides absolute position feedback.
    """

    PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1 = 1 << 4
    """
    Indicates that the device requires power supply across pin 1 (+) and pin 2 (-).
    """

    PBIO_IODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2 = 1 << 5
    """
    Indicates that the device requires power supply across pin 1 (-) and pin 2 (+).
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
    motor_type_id: IODeviceTypeId = IODeviceTypeId.NONE
    """
    The I/O device type ID of the motor (or non-UART light) connected to this port.

    This should be set to ``IODeviceTypeId.NONE`` when a non-motor sensor is
    attached or if nothing is attached.
    """

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
