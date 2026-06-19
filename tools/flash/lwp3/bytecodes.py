# SPDX-License-Identifier: MIT
# Copyright (c) 2021-2024 The Pybricks Authors
# Some portions of the documentation:
# Copyright (c) 2018 LEGO System A/S

"""
The LWP3 :mod:`.bytecodes` module contains enums for interpreting binary values
used in the `LWP3 protocol`_.

.. _LWP3 protocol: https://lego.github.io/lego-ble-wireless-protocol-docs/
"""

from enum import IntEnum, IntFlag, unique


def _create_pseudo_member_(cls: type[IntEnum], value: int) -> IntEnum:
    """
    Creates a new enum member at runtime for ``IntEnum``s.
    """
    pseudo_member = cls._value2member_map_.get(value, None)
    if pseudo_member is None:
        # construct singleton pseudo-members
        pseudo_member = int.__new__(cls, value)
        pseudo_member._name_ = str(value)
        pseudo_member._value_ = value
        # use setdefault in case another thread already created a composite
        # with this value
        pseudo_member = cls._value2member_map_.setdefault(value, pseudo_member)
    return pseudo_member


MAX_NAME_SIZE = 14
"""The maximum allowable size of the hub name in bytes."""


class Version(int):
    """An encoded version number."""

    @property
    def major(self) -> int:
        """Gets the major version component."""
        return (self >> 28) & 0xF

    @property
    def minor(self) -> int:
        """Gets the minor version component."""
        return (self >> 24) & 0xF

    @property
    def bug(self) -> int:
        """Gets the bug fix version component."""
        # uses BCD, so convert to string and back to int
        return int(f"{((self >> 16) & 0xFF):X}")

    @property
    def build(self) -> int:
        """Gets the bug fix version component."""
        # uses BCD, so convert to string and back to int
        return int(f"{(self & 0xFFFF):X}")

    @staticmethod
    def parse(version: str) -> "Version":
        """Parses a string with the format ``X.X.XX.XXXX``."""
        major, minor, bug, build = version.split(".")
        major = int(major) << 28
        minor = int(minor) << 24
        bug = int(bug, 16) << 16
        build = int(build, 16)
        return Version(major | minor | bug | build)

    def __str__(self) -> str:
        """Returns the version in ``X.X.XX.XXXX`` format."""
        ver = list(f"{self:08X}")
        ver.insert(4, ".")
        ver.insert(2, ".")
        ver.insert(1, ".")
        return "".join(ver)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(0x{self:08X})"


class LWPVersion(int):
    """The LEGO Wireless Protocol (LWP) version."""

    @property
    def major(self) -> int:
        """Gets the major version component."""
        # uses BCD, so convert to string and back to int
        return int(f"{((self >> 8) & 0xFF):X}")

    @property
    def minor(self) -> int:
        """Gets the minor version component."""
        # uses BCD, so convert to string and back to int
        return int(f"{(self & 0xFF):X}")

    @staticmethod
    def parse(version: str) -> "LWPVersion":
        """Parses a string with the format ``XX.XX``."""
        major, minor = version.split(".")
        major = int(major, 16) << 8
        minor = int(minor, 16)
        return LWPVersion(major | minor)

    def __str__(self) -> str:
        """Returns the version in ``XX.XX`` format."""
        ver = list(f"{self:04X}")
        ver.insert(2, ".")
        return "".join(ver)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(0x{self:04X})"


class BluetoothAddress(bytes):
    """
    A Bluetooth address.

    These addresses use the same EUI-48 format as MAC addresses but are for
    identifying individual Bluetooth devices instead of network cards.
    """

    def __new__(cls, value: str | bytes) -> "BluetoothAddress":
        if isinstance(value, str):
            # if it is a string, assume the format "XX:XX:XX:XX:XX:XX"
            value = [int(x, 16) for x in value.split(":")]

        if len(value) != 6:
            raise TypeError("requires exactly 6 bytes")

        return bytes.__new__(BluetoothAddress, value)

    def __str__(self) -> str:
        return ":".join(f"{b:02X}" for b in self)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(str(self))})"


@unique
class SystemKind(IntEnum):
    """
    Indicates the system that a hub belongs to.

    This is encoded in :class:`HubKind` and is not used as a bytecode on its own.
    """

    WEDO2 = 0 << 5
    """LEGO WeDo 2.0"""

    DUPLO = 1 << 5
    """LEGO Duplo"""

    SYSTEM = 2 << 5
    """LEGO System"""

    SYSTEM_ = 3 << 5
    """LEGO System"""

    TECHNIC = 4 << 5
    """LEGO Technic"""

    LEGACY = 7 << 5
    """Pre-Powered Up (unofficial Pybricks addition)"""


@unique
class HubKind(IntEnum):
    """
    Indicates the kind of hub.

    This is used both in advertising data an in messages.
    """

    WEDO2 = 0x00
    """LEGO WeDo 2.0 Hub."""

    DUPLO_TRAIN = 0x20
    """LEGO Duplo Train."""

    BOOST = 0x40
    """LEGO BOOST Move Hub."""

    CITY = 0x41
    """LEGO 2-port Hub."""

    HANDSET = 0x42
    """LEGO 2-port Handset (remote control)."""

    MARIO = 0x43
    """LEGO Mario Hub."""

    LUIGI = 0x44
    """LEGO Luigi Hub."""

    PEACH = 0x45
    """LEGO Peach Hub."""

    TECHNIC = 0x80
    """LEGO 4-port Technic Hub."""

    TECHNIC_LARGE = 0x81
    """LEGO SPIKE Prime Hub and MINDSTORMS Robot Inventor hubs."""

    TECHNIC_SMALL = 0x83
    """LEGO SPIKE Essential Hub."""

    TECHNIC_MOVE = 0x84
    """LEGO Technic Move Hub."""

    RCX = 0xE0
    """LEGO MINDSTORMS RCX brick. (unofficial Pybricks addition)"""

    NXT = 0xE1
    """LEGO MINDSTORMS NXT brick. (unofficial Pybricks addition)"""

    EV3 = 0xE2
    """LEGO MINDSTORMS EV3 brick. (unofficial Pybricks addition)"""

    @property
    def system(self) -> SystemKind:
        """
        Gets ths system that this hub belongs to.
        """
        # system is encoded in the first 3 bits
        return SystemKind(self & (7 << 5))


@unique
class Capabilities(IntFlag):
    """
    Describes the capabilities of a hub.

    These flags are used in advertising data.
    """

    CENTRAL = 1 << 0
    """The hub supports the central role."""

    PERIPHERAL = 1 << 1
    """The hub supports the peripheral role."""

    IO = 1 << 2
    """The hub has I/O ports for connecting sensors/motors."""

    REMOTE = 1 << 3
    """The hub acts as a remote control."""


@unique
class LastNetwork(IntEnum):
    """
    Describes the last network ID used in the LWP3 pairing process.

    This is used both in advertising data and messages.
    """

    NONE = 0
    """No connection has been made yet."""

    LOCKED = 251
    """Locked (default 1)."""

    NOT_LOCKED = 252
    """Not locked (default 2)."""

    RSSI = 253
    """RSSI dependant (default 3)."""

    DISABLE_HW_NET = 254
    """Disable hardware network (default 4)."""

    DONT_CARE = 255
    """Don't care (not implemented)."""

    @classmethod
    def _missing_(cls, value):
        # only 1 to 250 are valid "last connection" IDs
        if value <= cls.NONE or value >= cls.LOCKED:
            return None
        return _create_pseudo_member_(cls, value)


@unique
class Status(IntFlag):
    """
    Indicates the status of a connection request.

    This is used both in advertising data and in messages.
    """

    PERIPHERAL = 0x01
    """The requesting device can be a peripheral."""

    CENTRAL = 0x02
    """The requesting device can be a central."""

    REQUEST_WINDOW = 0x20
    """A button press is extending the request window."""

    REQUEST_CONNECT = 0x40
    """A hard-coded request."""


@unique
class BatteryKind(IntEnum):
    """The kind of battery."""

    NORMAL = 0x00
    """Standard AA or AAA batteries."""

    RECHARGEABLE = 0x01
    """Rechargeable Li-ion or Li-po battery pack."""


@unique
class MessageKind(IntEnum):
    """
    Indicates the kind of message.
    """

    # Hub related

    HUB_PROPERTY = 0x01
    """Messages that get or set hub a hub property."""

    HUB_ACTION = 0x02
    """Messages that perform a hub action."""

    HUB_ALERT = 0x03
    """Messages to subscribe or retrieve hub alerts."""

    HUB_ATTACHED_IO = 0x04
    """Messages received when an I/O device is attached."""

    ERROR = 0x05
    """Generic error messages."""

    HW_NET_CMD = 0x08
    """Commands used for hardware networks."""

    FW_UPDATE = 0x10
    """Message to put the hub in firmware update mode."""

    FW_LOCK = 0x011
    """Message to lock the bootloader flash memory (factory use)."""

    FW_LOCK_STATUS_REQ = 0x12
    """Message to request the bootloader flash memory lock state."""

    FW_LOCK_STATUS = 0x13
    """Reply to :attr:`FW_LOCK_STATUS_REQ`."""

    # port related

    PORT_INFO_REQ = 0x21
    """Messages that request port information."""

    PORT_MODE_INFO_REQ = 0x22
    """Messages tha request mode information."""

    PORT_INPUT_FMT_SETUP = 0x41
    """Message to set up input format for a single mode."""

    PORT_INPUT_FMT_SETUP_COMBO = 0x42
    """Message to set up input format for a mode combo."""

    PORT_INFO = 0x43
    """Reply to a :attr:`PORT_INFO_REQ`."""

    PORT_MODE_INFO = 0x44
    """Reply to a :attr:`PORT_MODE_INFO_REQ`."""

    PORT_VALUE = 0x45
    """Value update from a single mode."""

    PORT_VALUE_COMBO = 0x46
    """Value update from a mode combo."""

    PORT_INPUT_FMT = 0x47
    """Reply to a :attr:`PORT_INPUT_FMT_SETUP`."""

    PORT_INPUT_FMT_COMBO = 0x48
    """Reply to a :attr:`PORT_INPUT_FMT_SETUP_COMBO`."""

    VIRTUAL_PORT_SETUP = 0x61
    """Message to set up virtual port that provides synchronization of two ports."""

    PORT_OUTPUT_CMD = 0x81
    """Messages to execute port output commands."""

    PORT_OUTPUT_CMD_FEEDBACK = 0x82
    """Message that indicate a port output command has completed."""


@unique
class HubProperty(IntEnum):
    """
    Properties used in :attr:`MessageKind.HUB_PROPERTY` messages.
    """

    NAME = 0x01
    """The hub name."""

    BUTTON = 0x02
    """The button state."""

    FW_VERSION = 0x03
    """The firmware version."""

    HW_VERSION = 0x04
    """The hardware version."""

    RSSI = 0x05
    """Radio signal strength indication."""

    BATTERY_VOLTAGE = 0x06
    """Battery voltage (as percent)."""

    BATTERY_KIND = 0x07
    """Battery type."""

    MFG_NAME = 0x08
    """Manufacturer name."""

    RADIO_FW_VERSION = 0x09
    """Bluetooth radio firmware version."""

    LWP_VERSION = 0x0A
    """LEGO Wireless Protocol version."""

    HUB_KIND = 0x0B
    """The kind of hub."""

    HW_NET_ID = 0x0C
    """Hardware network ID."""

    BDADDR = 0x0D
    """Bluetooth address."""

    BOOTLOADER_BDADDR = 0x0E
    """Bootloader Bluetooth address."""

    HW_NET_FAMILY = 0x0F
    """Hardware network family."""

    VOLUME = 0x12
    """Sound volume level."""


@unique
class HubPropertyOperation(IntEnum):
    """
    Operations for hub property messages.
    """

    SET = 0x01
    """Sets a property."""

    ENABLE_UPDATES = 0x02
    """Enables continuous updates for a property."""

    DISABLE_UPDATES = 0x03
    """Disables continuous updates for a property."""

    RESET = 0x04
    """Resets a property to the default value."""

    REQUEST_UPDATE = 0x05
    """Requests a single property update."""

    UPDATE = 0x06
    """Contains the current value of a property."""


@unique
class HubAction(IntEnum):
    """
    Instructs the hub to perform an action.
    """

    # sent to hub

    POWER_OFF = 0x01
    """Switches the hub off."""

    DISCONNECT = 0x02
    """Instructs the hub to disconnect Bluetooth."""

    PORT_VCC_ON = 0x03
    """Turns on 3.3V on I/O port pin 4 for all ports."""

    PORT_VCC_OFF = 0x04
    """Turns off 3.3V on I/O port pin 4 for all ports."""

    SET_BUSY = 0x05
    """Sets busy indication on status light."""

    RESET_BUSY = 0x06
    """Resets busy indication on status light."""

    FAST_POWER_OFF = 0x2F
    """Fast power off (factory use)."""

    # received from hub

    WILL_POWER_OFF = 0x30
    """The hub will switch off."""

    WILL_DISCONNECT = 0x31
    """The hub will disconnect."""

    WILL_UPDATE = 0x32
    """The hub will restart in firmware update mode."""


@unique
class AlertKind(IntEnum):
    """
    Alert conditions received from the hub.
    """

    LOW_VOLTAGE = 0x01
    """The battery voltage is low."""

    HIGH_CURRENT = 0x02
    """The battery current is high."""

    LOW_SIGNAL = 0x03
    """The Bluetooth signal strength is low."""

    OVER_POWER = 0x04
    """The hub is using too much power."""


@unique
class AlertOperation(IntEnum):
    """
    Operations that can be done with hub alerts.
    """

    ENABLE_UPDATES = 0x01
    """Enables updates from the hub."""

    DISABLE_UPDATES = 0x02
    """Disables updates from the hub."""

    REQUEST_UPDATE = 0x03
    """Requests a single update from the hub."""

    UPDATE = 0x04
    """Contains an update received from the hub."""


@unique
class AlertStatus(IntEnum):
    """
    Indicates if the alert is present or not."""

    OK = 0x00
    """Status is OK."""

    ALERT = 0xFF
    """The alert condition is present."""


@unique
class PortID(IntEnum):
    """
    Represents the ID of an external or internal I/O port.

    All enum members are dynamic. (0 to 49 are external and 50 - 100 are internal)
    """

    names = ()

    @property
    def internal(self) -> bool:
        """Tests if the port is internal."""
        return 50 <= self <= 100

    @classmethod
    def _missing_(cls, value):
        # 8-bit value, 101-255 is reserved
        if value < 0 or value > 100:
            return None
        return _create_pseudo_member_(cls, value)


@unique
class IOEvent(IntEnum):
    """
    Events from I/O ports.
    """

    DETACHED = 0x00
    """An I/O device was detached."""

    ATTACHED = 0x01
    """An I/O device was attached."""

    ATTACHED_VIRTUAL = 0x02
    """A virtual I/O device was attached."""


@unique
class IODeviceKind(IntEnum):
    """
    The kind of attached I/O device.
    """

    NONE = 0x00
    """No device."""

    MEDIUM_MOTOR = 0x01
    """Powered Up Medium Motor."""

    TRAIN_MOTOR = 0x02
    """Powered Up Train Motor"""

    LIGHTS = 0x08
    """Powered Up Lights."""

    HUB_BATTERY_VOLTAGE = 0x14
    """Powered Up Hub battery voltage."""

    HUB_BATTERY_CURRENT = 0x15
    """Powered Up Hub battery current."""

    HUB_PIEZO = 0x16
    """Powered Up Hub piezo buzzer."""

    HUB_STATUS_LIGHT = 0x17
    """Powered Up Hub status light."""

    EV3_COLOR_SENSOR = 0x1D
    """EV3 Color Sensor."""

    EV3_ULTRASONIC_SENSOR = 0x1E
    """EV3 Ultrasonic Sensor."""

    EV3_GYRO_SENSOR = 0x20
    """EV3 Gyro Sensor."""

    EV3_IR_SENSOR = 0x21
    """EV3 Infrared Sensor."""

    WEDO_TILT_SENSOR = 0x22
    """WeDo 2.0 Tilt Sensor."""

    WEDO_MOTION_SENSOR = 0x23
    """WeDo 2.0 Motion Sensor."""

    WEDO_GENERIC = 0x24
    """WeDo 2.0 generic device."""

    BOOST_COLOR_DISTANCE_SENSOR = 0x25
    """BOOST Color and Distance Sensor"""

    BOOST_INTERACTIVE_MOTOR = 0x26
    """BOOST Interactive Motor"""

    BOOST_HUB_MOTOR = 0x27
    """BOOST Move Hub built-in motor."""

    BOOST_HUB_ACCEL = 0x28
    """BOOST Move Hub built-in accelerometer (tilt sensor)."""

    DUPLO_TRAIN_MOTOR = 0x29
    """DUPLO Train hub built-in motor."""

    DUPLO_TRAIN_BEEPER = 0x2A
    """DUPLO Train hub built-in beeper."""

    DUPLO_TRAIN_COLOR_SENSOR = 0x2B
    """DUPLO Train hub built-in color sensor."""

    DUPLO_TRAIN_SPEED = 0x2C
    """DUPLO Train hub built-in speed sensor."""

    TECHNIC_LARGE_MOTOR = 0x2E
    """Technic Control+ Large Motor."""

    TECHNIC_XL_MOTOR = 0x2F
    """Technic Control+ XL Motor."""

    SPIKE_MEDIUM_MOTOR = 0x30
    """SPIKE Prime Medium Motor."""

    SPIKE_LARGE_MOTOR = 0x31
    """SPIKE Prime Large Motor."""

    HUB_UNKNOWN_X32 = 0x32
    """Technic Control+ Hub ?"""

    HUB_IMU_GESTURE = 0x36
    """Powered Up hub built-int IMU gesture."""

    REMOTE_BUTTONS = 0x37
    """Powered Up Handset Buttons."""

    HUB_RSSI = 0x38
    """Powered Up hub Bluetooth RSSI"""

    HUB_IMU_ACCEL = 0x39
    """Powered Up hub built-in IMU accelerometer."""

    HUB_IMU_GYRO = 0x3A
    """Powered Up hub built-in IMU gyro"""

    HUB_IMU_ORIENTATION = 0x3B
    """Powered Up hub built-in IMU orientation."""

    HUB_IMU_TEMPERATURE = 0x3C
    """Powered Up hub built-in IMU temperature."""

    SPIKE_COLOR_SENSOR = 0x3D
    """SPIKE/MINDSTORMS Color Sensor."""

    SPIKE_ULTRASONIC_SENSOR = 0x3E
    """SPIKE/MINDSTORMS Ultrasonic Distance Sensor."""

    SPIKE_FORCE_SENSOR = 0x3F
    """SPIKE/MINDSTORMS Force Sensor."""

    TECHNIC_MEDIUM_ANGULAR_MOTOR = 0x4B
    """Technic Medium Angular Motor, gray."""

    TECHNIC_LARGE_ANGULAR_MOTOR = 0x4C
    """Technic Large Angular motor, gray."""

    @classmethod
    def _missing_(cls, value):
        # allow missing here for home-made devices
        if value < 0 or value > 65535:
            return None
        return _create_pseudo_member_(cls, value)


@unique
class ErrorCode(IntEnum):
    """
    Generic error codes.
    """

    ACK = 0x01

    NACK = 0x02

    BUFFER_OVERFLOW = 0x03

    TIMEOUT = 0x04

    UNKNOWN_COMMAND = 0x05
    """The command is not supported."""

    INVALID = 0x06
    """Invalid use (e.g. invalid parameters)."""

    OVER_CURRENT = 0x07

    INTERNAL_ERROR = 0x08


@unique
class HwNetCmd(IntEnum):
    """
    Hardware network commands.

    These commands are for creating hub-to-hub connections, not hub to phone/tablet/computer
    connections.
    """

    CONNECTION_REQUEST = 0x02
    """
    This message is used to route/gate a connection request from a peripheral
    device up to the controlling Central device. The user should be able to
    press the button on any devices in the current connected network for
    requesting a connection. Both the Pressed and the Release is send.
    """

    FAMILY_REQUEST = 0x03
    """
    When send upstream (from any device in the H/W network) this message is a
    request for a new Family and SubFamily, if possible.
    """

    FAMILY_SET = 0x04
    """
    This command is send from the controlling Central device and has the new
    family added as payload.
    """

    JOIN_DENIED = 0x05
    """
    This command is send to a peripheral trying to connect to a network where
    the maximum of nodes is reached (i.e. no further connections available).
    """

    GET_FAMILY = 0x06
    """
    Requesting the previous used Family for a specific (the addressed) device,
    just entered a network - no family decided yet.
    """

    FAMILY = 0x07
    """
    This is the answer returned for above command (Get Family 0x06). Used by the
    Central Role device to decide the Family at auto connection.
    """

    GET_SUBFAMILY = 0x08
    """
    Requesting the previous used SubFamily (Function) on a specific (addressed)
    device. E.g. a 2 port R/C to a 4 port hub. I.e. addressing a specific port
    (A+B or C+D)
    """

    SUBFAMILY = 0x09
    """
    This is the answer returned for above command (Get SubFamily 0x08). Used by
    the Central Role device to decide the SubFamily at auto connection.
    """

    SUBFAMILY_SET = 0x0A
    """
    This command is send from the controlling Central device and has the new
    SubFamily added as payload.
    """

    GET_EXTENDED_FAMILY = 0x0B
    """
    Requesting the previous used Family and SubFamily for a specific (the
    addressed) device. The user can get both the Family and the SubFamily in
    one tx/rx. Further both are only using 3 bits for the addressing leaving
    one bit each for future use and/or escape.
    """

    EXTENDED_FAMILY = 0x0C
    """
    This is the answer returned for above command (Get Extended Family 0x0A).
    """

    EXTENDED_FAMILY_SET = 0x0D
    """
    The user can set both the Family and the SubFamily in one tx/rx. Further
    both are only using 3 bits for the addressing leaving one bit each for
    future use and/or escape (extended use).
    """

    RESET_LONG_PRESS = 0x0E
    """
    The timing of the user CONNECTION PRESS may be stretched (timer reset) when
    connecting in a Bluetooth active environment. I.e. the LONG PRESS normally
    is used for powering down a device, but also used for gating key presses to
    the Central in a connection sequence (allowing a longer connection time
    without powering down.
    """


class HwNetFamily(IntEnum):
    """
    Hardware network family.

    Families are denoted by the status light color.
    """

    WHITE = 0x00
    GREEN = 0x01
    YELLOW = 0x02
    RED = 0x03
    BLUE = 0x04
    PURPLE = 0x05
    CYAN = 0x06
    TEAL = 0x07
    PINK = 0x08

    def __add__(self, x: int) -> int:
        if isinstance(x, HwNetSubfamily):
            return HwNetExtFamily.from_parts(self, x)

        return super().__add__(x)


class HwNetSubfamily(IntEnum):
    """
    Hardware network subfamily.

    Subfamilies are denoted by the number of flashes of the status light.
    """

    FLASH_0 = 0x00
    FLASH_1 = 0x01
    FLASH_2 = 0x02
    FLASH_3 = 0x03
    FLASH_4 = 0x04
    FLASH_5 = 0x05
    FLASH_6 = 0x06
    FLASH_7 = 0x07

    def __add__(self, x: int) -> int:
        if isinstance(x, HwNetFamily):
            return HwNetExtFamily.from_parts(x, self)

        return super().__add__(x)


class HwNetExtFamily(IntEnum):
    """
    Combination of :class:`HwNetFamily` and :class:`HwNetSubfamily` encoded in
    a single byte.
    """

    names = ()

    @property
    def family(self) -> HwNetFamily:
        return HwNetFamily(self & 0x0F)

    @property
    def subfamily(self) -> HwNetSubfamily:
        return HwNetSubfamily((self >> 4) & 0x07)

    @staticmethod
    def from_parts(family: HwNetFamily, subfamily: HwNetSubfamily) -> "HwNetExtFamily":
        return HwNetExtFamily((subfamily << 4) | family)

    @classmethod
    def _missing_(cls, value):
        try:
            family = HwNetFamily(value & 0xF)
            subfamily = HwNetSubfamily((value >> 4) & 0x7)
            if ((subfamily << 4) | family) != value:
                return None
        except Exception as ex:
            return ex

        return _create_pseudo_member_(cls, value)

    def __repr__(self) -> str:
        return f"({repr(self.family)} + {repr(self.subfamily)})"


class InfoKind(IntEnum):
    PORT_VALUE = 0x00
    MODE_INFO = 0x01
    COMBOS = 0x02


class ModeInfoKind(IntEnum):
    NAME = 0x00
    RAW = 0x01
    PCT = 0x02
    SI = 0x03
    SYMBOL = 0x04
    MAPPING = 0x05
    INTERNAL_USE = 0x06
    MOTOR_BIAS = 0x07
    CAPABILITIES = 0x08
    UNK9 = 0x09
    UNK10 = 0x0A
    UNK11 = 0x0B
    UNK12 = 0x0C
    FORMAT = 0x80


class PortInfoFormatSetupCommand(IntEnum):
    SET = 0x01
    """Set mode and data format combos."""

    LOCK = 0x02
    """Lock I/O device for setup."""

    UNLOCK_ENABLED = 0x03
    """Unlock and start with updates enabled."""

    UNLOCK_DISABLED = 0x04
    """Unlock and start with updates disabled."""

    RESERVED = 0x05
    """Not used."""

    RESET = 0x06
    """Reset I/O device."""


class ModeCapabilities(IntFlag):
    OUTPUT = 1 << 0
    INPUT = 1 << 1
    LOGICAL_COMBINABLE = 1 << 2
    LOGICAL_SYNCHRONIZEABLE = 1 << 3


class IODeviceMapping(IntFlag):
    DISCRETE = 1 << 2
    RELATIVE = 1 << 3
    ABSOLUTE = 1 << 4
    SUPPORTS_MAPPING_V2 = 1 << 6
    SUPPORTS_NULL = 1 << 7


class IODeviceCapabilities(IntFlag):
    """
    Sensor capabilities flags. (48-bit)
    """

    names = ()


class DataFormat(IntEnum):
    """
    I/O Device data format.
    """

    DATA8 = 0x00
    """8-bit signed integer."""

    DATA16 = 0x01
    """16-bit signed integer, little-endian."""

    DATA32 = 0x02
    """32-bit signed integer, little-endian."""

    DATAF = 0x03
    """32-bit floating point, little-endian."""


class VirtualPortSetupCommand(IntEnum):
    DISCONNECT = 0x00
    CONNECT = 0x01


class PortOutputCommand(IntEnum):
    START_POWER = 0x01
    START_POWER_2 = 0x02
    SET_ACC_TIME = 0x05
    SET_DEC_TIME = 0x06
    START_SPEED = 0x07
    START_SPEED_2 = 0x08
    START_SPEED_FOR_TIME = 0x09
    START_SPEED_FOR_TIME_2 = 0x0A
    START_SPEED_FOR_DEGREES = 0x0B
    START_SPEED_FOR_DEGREES_2 = 0x0C
    GOTO_ABS_POS = 0x0D
    GOTO_ABS_POS_2 = 0x0E
    PRESET_ENCODER = 0x13
    PRESET_ENCODER_2 = 0x14
    WRITE_DIRECT = 0x50
    WRITE_DIRECT_MODE_DATA = 0x51


class StartInfo(IntEnum):
    BUFFER = 0x00
    IMMEDIATE = 0x10


class EndInfo(IntEnum):
    NO_ACTION = 0x00
    FEEDBACK = 0x01


class Feedback(IntFlag):
    BUFFER_EMPTY_IN_PROGRESS = 1 << 0
    BUFFER_EMPTY_COMPLETED = 1 << 1
    DISCARDED = 1 << 2
    IDLE = 1 << 3
    BUSY = 1 << 4
