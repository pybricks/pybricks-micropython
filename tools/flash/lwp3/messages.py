# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors
# Some portions of the documentation:
# Copyright (c) 2018 LEGO System A/S

"""
The LWP3 :mod:`.messages` module contains classes for encoding and decoding
messages used in the `LWP3 protocol`_.

.. _LWP3 protocol: https://lego.github.io/lego-ble-wireless-protocol-docs/
"""

import abc
import struct
from enum import IntEnum
from typing import Any, NamedTuple, overload

from lwp3.bytecodes import (
    MAX_NAME_SIZE,
    AlertKind,
    AlertOperation,
    AlertStatus,
    BatteryKind,
    BluetoothAddress,
    DataFormat,
    EndInfo,
    ErrorCode,
    Feedback,
    HubAction,
    HubKind,
    HubProperty,
    HubPropertyOperation,
    HwNetCmd,
    HwNetExtFamily,
    HwNetFamily,
    HwNetSubfamily,
    InfoKind,
    IODeviceCapabilities,
    IODeviceKind,
    IODeviceMapping,
    IOEvent,
    LastNetwork,
    LWPVersion,
    MessageKind,
    ModeCapabilities,
    ModeInfoKind,
    PortID,
    PortInfoFormatSetupCommand,
    PortOutputCommand,
    StartInfo,
    Version,
    VirtualPortSetupCommand,
)
from checksum import xor_bytes


class AbstractMessage(abc.ABC):
    """Common base class for all messages."""

    @abc.abstractmethod
    def __init__(self, length: int, kind: MessageKind) -> None:
        super().__init__()

        if not isinstance(length, int):
            raise TypeError("length must be int")

        if not isinstance(kind, MessageKind):
            raise TypeError("kind must be MessageKind")

        self._data = bytearray(length)
        self._data[0] = length
        self._data[2] = kind

    def __bytes__(self) -> bytes:
        return bytes(self._data)

    @property
    def length(self) -> int:
        """Gets the length of the message in bytes."""
        return self._data[0]

    @property
    def kind(self) -> MessageKind:
        """Gets the kind of message."""
        return MessageKind(self._data[2])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}()"


###############################################################################
# Hub property messages
###############################################################################


class AbstractHubPropertyMessage(AbstractMessage):
    """Common base class for hub property messages."""

    @abc.abstractmethod
    def __init__(
        self, length: int, prop: HubProperty, op: HubPropertyOperation
    ) -> None:
        """
        Args:
            length: Length of the message in bytes.
            prop: The property.
            op: The operation to perform.

        Raises:
            TypeError:
                ``prop`` is not a :class:`.bytecodes.HubProperty` or ``op`` is
                not a :class:`.bytecodes.HubPropertyOperation`.
            ValueError:
                ``op`` cannot be applied to ``prop``
        """
        super().__init__(length, MessageKind.HUB_PROPERTY)

        if not isinstance(prop, HubProperty):
            raise TypeError("prop must be HubProperty")

        if op not in _HUB_PROPERTY_OPS_MAP[prop]:
            raise ValueError(f"cannot perform {op} on {prop}")

        if not isinstance(op, HubPropertyOperation):
            raise TypeError("op must be HubPropertyOperation")

        self._data[3] = prop
        self._data[4] = op

    @property
    def prop(self) -> HubProperty:
        """Gets the property that is acted on."""
        return HubProperty(self._data[3])

    @property
    def op(self) -> HubPropertyOperation:
        """Gets the operation."""
        return HubPropertyOperation(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.prop)})"


class _HubPropertyType(NamedTuple):
    type: type
    fmt: str
    max_size: int | None = None


# specifies payload type information for each property
_HUB_PROPERTY_TYPE_MAP = {
    HubProperty.NAME: _HubPropertyType(str, "s", MAX_NAME_SIZE),
    HubProperty.BUTTON: _HubPropertyType(bool, "?"),
    HubProperty.FW_VERSION: _HubPropertyType(Version, "i"),
    HubProperty.HW_VERSION: _HubPropertyType(Version, "i"),
    HubProperty.RSSI: _HubPropertyType(int, "b"),
    HubProperty.BATTERY_VOLTAGE: _HubPropertyType(int, "B"),
    HubProperty.BATTERY_KIND: _HubPropertyType(BatteryKind, "B"),
    HubProperty.MFG_NAME: _HubPropertyType(str, "s", 15),
    HubProperty.RADIO_FW_VERSION: _HubPropertyType(str, "s", 15),
    HubProperty.LWP_VERSION: _HubPropertyType(LWPVersion, "H"),
    HubProperty.HUB_KIND: _HubPropertyType(HubKind, "B"),
    HubProperty.HW_NET_ID: _HubPropertyType(LastNetwork, "B"),
    HubProperty.BDADDR: _HubPropertyType(BluetoothAddress, "6s"),
    HubProperty.BOOTLOADER_BDADDR: _HubPropertyType(BluetoothAddress, "6s"),
    HubProperty.HW_NET_FAMILY: _HubPropertyType(HwNetFamily, "B"),
    HubProperty.VOLUME: _HubPropertyType(int, "B"),
}

Op = HubPropertyOperation

# specifies supported operations for each property
_HUB_PROPERTY_OPS_MAP = {
    HubProperty.NAME: [
        Op.SET,
        Op.ENABLE_UPDATES,
        Op.DISABLE_UPDATES,
        Op.RESET,
        Op.REQUEST_UPDATE,
        Op.UPDATE,
    ],
    HubProperty.BUTTON: [
        Op.ENABLE_UPDATES,
        Op.DISABLE_UPDATES,
        Op.REQUEST_UPDATE,
        Op.UPDATE,
    ],
    HubProperty.FW_VERSION: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.HW_VERSION: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.RSSI: [
        Op.ENABLE_UPDATES,
        Op.DISABLE_UPDATES,
        Op.REQUEST_UPDATE,
        Op.UPDATE,
    ],
    HubProperty.BATTERY_VOLTAGE: [
        Op.ENABLE_UPDATES,
        Op.DISABLE_UPDATES,
        Op.REQUEST_UPDATE,
        Op.UPDATE,
    ],
    HubProperty.BATTERY_KIND: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.MFG_NAME: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.RADIO_FW_VERSION: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.LWP_VERSION: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.HUB_KIND: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.HW_NET_ID: [Op.SET, Op.RESET, Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.BDADDR: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.BOOTLOADER_BDADDR: [Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.HW_NET_FAMILY: [Op.SET, Op.REQUEST_UPDATE, Op.UPDATE],
    HubProperty.VOLUME: [
        Op.SET,
        Op.ENABLE_UPDATES,
        Op.DISABLE_UPDATES,
        Op.RESET,
        Op.REQUEST_UPDATE,
        Op.UPDATE,
    ],
}

del Op


class AbstractHubPropertyValueMessage(AbstractHubPropertyMessage):
    """Common base class for hub property messages that have a value parameter."""

    _MAX_VALUE_SIZE = 15  # largest known value size

    @abc.abstractmethod
    def __init__(self, prop: HubProperty, op: HubPropertyOperation, value: Any) -> None:
        """
        Args:
            prop: The property.
            value: The new value.

        Raises:
            TypeError: ``value`` is not the correct type for ``prop``.
            ValueError: ``prop`` cannot be set.
        """
        # allocate enough for max size - length will be adjusted later
        super().__init__(5 + self._MAX_VALUE_SIZE, prop, op)

        meta = _HUB_PROPERTY_TYPE_MAP[self.prop]

        if not isinstance(value, meta.type):
            raise TypeError(
                f"expecting value of type {meta.type} but received {type(value)}"
            )

        if meta.max_size is None:
            # fixed size
            fmt = meta.fmt
        else:
            # variable size
            if isinstance(value, str):
                value = value.encode()

            if len(value) > meta.max_size:
                raise ValueError("length of value is too long")

            fmt = f"{len(value)}{meta.fmt}"

        # override the length
        self._data[0] = 5 + struct.calcsize(fmt)
        self._data = memoryview(self._data)[: self.length]

        struct.pack_into(fmt, self._data, 5, value)

    @property
    def value(self) -> Any:
        """Gets the property value."""

        meta = _HUB_PROPERTY_TYPE_MAP[self.prop]

        if meta.max_size is None:
            fmt = meta.fmt
        else:
            fmt = f"{self.length - 5}{meta.fmt}"

        (result,) = struct.unpack_from(fmt, self._data, 5)

        if meta.type == str:
            return result.decode().strip("\0")

        return meta.type(result)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.prop)}, {repr(self.value)})"


class HubPropertySet(AbstractHubPropertyValueMessage):
    """Hub property set message."""

    def __init__(self, prop: HubProperty, value: Any) -> None:
        """
        Args:
            prop: The property.
            value: The new value.

        Raises:
            TypeError: ``value`` is not the correct type for ``prop``.
            ValueError: ``prop`` cannot be set.
        """
        super().__init__(prop, HubPropertyOperation.SET, value)


class HubPropertyEnableUpdates(AbstractHubPropertyMessage):
    """Hub property enable updates message."""

    def __init__(self, prop: HubProperty) -> None:
        """
        Args:
            prop: The property.

        Raises:
            ValueError: ``prop`` does not allow enabling updates.
        """
        super().__init__(5, prop, HubPropertyOperation.ENABLE_UPDATES)


class HubPropertyDisableUpdates(AbstractHubPropertyMessage):
    """Hub property disable updates message."""

    def __init__(self, prop: HubProperty) -> None:
        """
        Args:
            prop: The property.

        Raises:
            ValueError: ``prop`` does not allow disabling updates.
        """
        super().__init__(5, prop, HubPropertyOperation.DISABLE_UPDATES)


class HubPropertyReset(AbstractHubPropertyMessage):
    """Hub property reset message."""

    def __init__(self, prop: HubProperty) -> None:
        """
        Args:
            prop: The property.

        Raises:
            ValueError: ``prop`` does not allow reset.
        """
        super().__init__(5, prop, HubPropertyOperation.RESET)


class HubPropertyRequestUpdate(AbstractHubPropertyMessage):
    """Hub property request update message."""

    def __init__(self, prop: HubProperty) -> None:
        """
        Args:
            prop: The property.
        """
        super().__init__(5, prop, HubPropertyOperation.REQUEST_UPDATE)


class HubPropertyUpdate(AbstractHubPropertyValueMessage):
    """Hub property update message."""

    def __init__(self, prop: HubProperty, value: Any) -> None:
        """
        Args:
            prop: The property.
            value: The new value.

        Raises:
            TypeError: if ``value`` is not the correct type for ``prop``.
        """
        super().__init__(prop, HubPropertyOperation.UPDATE, value)


###############################################################################
# Hub action messages
###############################################################################


class HubActionMessage(AbstractMessage):
    """
    This message allows for performing control actions on the connected Hub.
    """

    def __init__(self, action: HubAction) -> None:
        """
        Args:
            action: The action.
        """
        super().__init__(4, MessageKind.HUB_ACTION)

        self._data[3] = action

    @property
    def action(self) -> HubAction:
        """Gets the action."""
        return HubAction(self._data[3])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.action)})"


###############################################################################
# Hub alert messages
###############################################################################


class AbstractHubAlertMessage(AbstractMessage):
    """Common base type for all hub alert messages."""

    @abc.abstractmethod
    def __init__(self, length: int, alert: AlertKind, op: AlertOperation) -> None:
        super().__init__(length, MessageKind.HUB_ALERT)

        self._data[3] = alert
        self._data[4] = op

    @property
    def alert(self) -> AlertKind:
        """Gets the kind of alert."""
        return AlertKind(self._data[3])

    @property
    def op(self) -> AlertOperation:
        """Gets the operation to be performed."""
        return AlertOperation(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.alert)})"


class HubAlertEnableUpdatesMessage(AbstractHubAlertMessage):
    """
    Message to subscribe to updates for an alert.
    """

    def __init__(self, alert: AlertKind) -> None:
        super().__init__(5, alert, AlertOperation.ENABLE_UPDATES)


class HubAlertDisableUpdatesMessage(AbstractHubAlertMessage):
    """
    Message to unsubscribe from updates for an alert.
    """

    def __init__(self, alert: AlertKind) -> None:
        super().__init__(5, alert, AlertOperation.DISABLE_UPDATES)


class HubAlertRequestUpdateMessage(AbstractHubAlertMessage):
    """
    Message to request the current status for an alert.
    """

    def __init__(self, alert: AlertKind) -> None:
        super().__init__(5, alert, AlertOperation.REQUEST_UPDATE)


class HubAlertUpdateMessage(AbstractHubAlertMessage):
    """
    Message that contains the current status of an alert.
    """

    def __init__(self, alert: AlertKind, status: AlertStatus) -> None:
        super().__init__(6, alert, AlertOperation.UPDATE)

        self._data[5] = status

    @property
    def status(self) -> AlertStatus:
        """Gets the status of the alert."""
        return AlertStatus(self._data[5])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.alert)}, {repr(self.status)})"


###############################################################################
# Hub attached I/O messages
###############################################################################


class AbstractHubAttachedIOMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(self, length: int, port: PortID, event: IOEvent) -> None:
        super().__init__(length, MessageKind.HUB_ATTACHED_IO)

        self._data[3] = port
        self._data[4] = event

    @property
    def port(self) -> PortID:
        """Gets the I/O port ID."""
        return PortID(self._data[3])

    @property
    def event(self) -> IOEvent:
        """Gets the I/O port event."""
        return IOEvent(self._data[4])


class HubIODetachedMessage(AbstractHubAttachedIOMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, port, IOEvent.DETACHED)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)})"


class HubIOAttachedMessage(AbstractHubAttachedIOMessage):
    def __init__(
        self, port: PortID, device: IODeviceKind, hw_ver: Version, fw_ver: Version
    ) -> None:
        super().__init__(15, port, IOEvent.ATTACHED)

        struct.pack_into("<Hii", self._data, 5, device, hw_ver, fw_ver)

    @property
    def device(self) -> IODeviceKind:
        """Gets the kind of device that is attached."""
        (result,) = struct.unpack_from("<H", self._data, 5)
        return IODeviceKind(result)

    @property
    def hw_ver(self) -> Version:
        """Gets the hardware version of the device."""
        (result,) = struct.unpack_from("<i", self._data, 7)
        return Version(result)

    @property
    def fw_ver(self) -> Version:
        """Gets the firmware version of the device."""
        (result,) = struct.unpack_from("<i", self._data, 11)
        return Version(result)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.device)}, {repr(self.hw_ver)}, {repr(self.fw_ver)})"


class HubIOAttachedVirtualMessage(AbstractHubAttachedIOMessage):
    def __init__(
        self, port: PortID, device: IODeviceKind, port_a: PortID, port_b: PortID
    ) -> None:
        super().__init__(9, port, IOEvent.ATTACHED_VIRTUAL)

        struct.pack_into("<HBB", self._data, 5, device, port_a, port_b)

    @property
    def device(self) -> IODeviceKind:
        """Gets the kind of device that is attached."""
        (result,) = struct.unpack_from("<H", self._data, 5)
        return IODeviceKind(result)

    @property
    def port_a(self) -> Version:
        """Gets the first port of the virtual device."""
        return PortID(self._data[7])

    @property
    def port_b(self) -> Version:
        """Gets the second port of the virtual device."""
        return PortID(self._data[8])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.device)}, {repr(self.port_a)}, {repr(self.port_b)})"


###############################################################################
# Generic error messages
###############################################################################


class ErrorMessage(AbstractMessage):
    """Generic error message."""

    def __init__(self, command: MessageKind, code: ErrorCode) -> None:
        """
        Args:
            command: The kind of message that triggered the error.
            code: An error code describing the error.
        """
        super().__init__(5, MessageKind.ERROR)

        self._data[3] = command
        self._data[4] = code

    @property
    def command(self) -> MessageKind:
        """Gets the kind of message that triggered the error."""
        return MessageKind(self._data[3])

    @property
    def code(self) -> ErrorCode:
        """Gets an error code describing the error."""
        return ErrorCode(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.command)}, {repr(self.code)})"


###############################################################################
# Hardware network command messages
###############################################################################


class AbstractHwNetCmdMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(self, length: int, cmd: HwNetCmd) -> None:
        super().__init__(length, MessageKind.HW_NET_CMD)

        self._data[3] = cmd

    @property
    def cmd(self) -> HwNetCmd:
        return HwNetCmd(self._data[3])


class HwNetCmdRequestConnectionMessage(AbstractHwNetCmdMessage):
    def __init__(self, button_pressed: bool) -> None:
        super().__init__(5, HwNetCmd.CONNECTION_REQUEST)

        self._data[4] = button_pressed

    @property
    def button_pressed(self) -> bool:
        return bool(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.button_pressed)})"


class HwNetCmdRequestFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.FAMILY_REQUEST)


class HwNetCmdSetFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, family: HwNetFamily) -> None:
        super().__init__(5, HwNetCmd.FAMILY_SET)

        self._data[4] = family

    @property
    def family(self) -> HwNetFamily:
        return HwNetFamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.family)})"


class HwNetCmdJoinDeniedMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.JOIN_DENIED)


class HwNetCmdGetFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.GET_FAMILY)


class HwNetCmdFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, family: HwNetFamily) -> None:
        super().__init__(5, HwNetCmd.FAMILY)

        self._data[4] = family

    @property
    def family(self) -> HwNetFamily:
        return HwNetFamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.family)})"


class HwNetCmdGetSubfamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.GET_SUBFAMILY)


class HwNetCmdSubfamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, subfamily: HwNetSubfamily) -> None:
        super().__init__(5, HwNetCmd.SUBFAMILY)

        self._data[4] = subfamily

    @property
    def subfamily(self) -> HwNetSubfamily:
        return HwNetSubfamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.subfamily)})"


class HwNetCmdSetSubfamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, subfamily: HwNetSubfamily) -> None:
        super().__init__(5, HwNetCmd.SUBFAMILY_SET)

        self._data[4] = subfamily

    @property
    def subfamily(self) -> HwNetSubfamily:
        return HwNetSubfamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.subfamily)})"


class HwNetCmdGetExtendedFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.GET_EXTENDED_FAMILY)


class HwNetCmdExtendedFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, family: HwNetFamily, subfamily: HwNetSubfamily) -> None:
        super().__init__(5, HwNetCmd.EXTENDED_FAMILY)

        self._data[4] = family + subfamily

    @property
    def ext_family(self) -> HwNetExtFamily:
        return HwNetExtFamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.ext_family.family)}, {repr(self.ext_family.subfamily)})"


class HwNetCmdSetExtendedFamilyMessage(AbstractHwNetCmdMessage):
    def __init__(self, family: HwNetFamily, subfamily: HwNetSubfamily) -> None:
        super().__init__(5, HwNetCmd.EXTENDED_FAMILY_SET)

        self._data[4] = family + subfamily

    @property
    def ext_family(self) -> HwNetExtFamily:
        return HwNetExtFamily(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.ext_family.family)}, {repr(self.ext_family.subfamily)})"


class HwNetCmdResetLongPressMessage(AbstractHwNetCmdMessage):
    def __init__(self) -> None:
        super().__init__(4, HwNetCmd.RESET_LONG_PRESS)


###############################################################################
# Firmware/bootloader messages
###############################################################################


class FirmwareUpdateMessage(AbstractMessage):
    """
    Instructs the hub to reboot in firmware update mode.
    """

    def __init__(self) -> None:
        super().__init__(12, MessageKind.FW_UPDATE)

        self._data[3:] = b"LPF2-Boot"

    @property
    def key(self) -> bytes:
        """Safety string."""
        return bytes(self._data[3:])


###############################################################################
# Port info messages
###############################################################################


class PortInfoRequestMessage(AbstractMessage):
    def __init__(self, port: PortID, info_kind: InfoKind) -> None:
        super().__init__(5, MessageKind.PORT_INFO_REQ)

        self._data[3] = port
        self._data[4] = info_kind

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def info_kind(self) -> InfoKind:
        return InfoKind(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.info_kind)})"


class PortModeInfoRequestMessage(AbstractMessage):
    def __init__(self, port: PortID, mode: int, info_kind: ModeInfoKind) -> None:
        super().__init__(6, MessageKind.PORT_MODE_INFO_REQ)

        self._data[3] = port
        self._data[4] = mode
        self._data[5] = info_kind

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def mode(self) -> int:
        return self._data[4]

    @property
    def info_kind(self) -> InfoKind:
        return ModeInfoKind(self._data[5])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.info_kind)})"


class PortInputFormatSetupMessage(AbstractMessage):
    def __init__(self, port: PortID, mode: int, delta: int, notify: bool) -> None:
        super().__init__(10, MessageKind.PORT_INPUT_FMT_SETUP)

        struct.pack_into("<BBI?", self._data, 3, port, mode, delta, notify)

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def mode(self) -> int:
        return self._data[4]

    @property
    def delta(self) -> int:
        return struct.unpack_from("<I", self._data, 5)[0]

    @property
    def notify(self) -> bool:
        return bool(self._data[9])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.delta)}, {repr(self.notify)})"


class AbstractPortFormatSetupComboMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(
        self, length: int, port: PortID, command: PortInfoFormatSetupCommand
    ) -> None:
        super().__init__(length, MessageKind.PORT_INPUT_FMT_SETUP_COMBO)

        self._data[3] = port
        self._data[4] = command

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def command(self) -> PortInfoFormatSetupCommand:
        return PortInfoFormatSetupCommand(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)})"


class PortFormatSetupComboMessage(AbstractPortFormatSetupComboMessage):
    def __init__(self, port: PortID, modes_and_datasets: list[tuple[int, int]]) -> None:
        super().__init__(
            5 + len(modes_and_datasets), port, PortInfoFormatSetupCommand.SET
        )

        for i, (mode, dataset) in enumerate(modes_and_datasets, 5):
            self._data[i] = ((mode & 0xF) << 4) | (dataset & 0xF)

    @property
    def modes_and_datasets(self) -> list[tuple[int, int]]:
        return [(x >> 4, x & 0xF) for x in self._data[5:]]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.modes_and_datasets)})"


class PortFormatSetupComboLockMessage(AbstractPortFormatSetupComboMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, port, PortInfoFormatSetupCommand.LOCK)


class PortFormatSetupComboUnlockEnabledMessage(AbstractPortFormatSetupComboMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, port, PortInfoFormatSetupCommand.UNLOCK_ENABLED)


class PortFormatSetupComboUnlockDisabledMessage(AbstractPortFormatSetupComboMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, port, PortInfoFormatSetupCommand.UNLOCK_DISABLED)


class PortFormatSetupComboResetMessage(AbstractPortFormatSetupComboMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, port, PortInfoFormatSetupCommand.RESET)


class AbstractPortInfoMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(self, length: int, port: PortID, info_kind: InfoKind) -> None:
        super().__init__(length, MessageKind.PORT_INFO)

        if info_kind == InfoKind.PORT_VALUE:
            raise ValueError("info_kind can't be InfoKind.PORT_VALUE")

        self._data[3] = port
        self._data[4] = info_kind

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def info_kind(self) -> InfoKind:
        return InfoKind(self._data[4])


class PortInfoModeInfoMessage(AbstractPortInfoMessage):
    def __init__(
        self,
        port: PortID,
        capabilities: ModeCapabilities,
        num_modes: int,
        input_modes: list[int],
        output_modes: list[int],
    ) -> None:
        super().__init__(11, port, InfoKind.MODE_INFO)

        input_mode_flags = 0
        for m in input_modes:
            input_mode_flags |= 1 << m

        output_mode_flags = 0
        for m in output_modes:
            output_mode_flags |= 1 << m

        struct.pack_into(
            "<BBHH",
            self._data,
            5,
            capabilities,
            num_modes,
            input_mode_flags,
            output_mode_flags,
        )

    @property
    def capabilities(self) -> ModeCapabilities:
        return ModeCapabilities(self._data[5])

    @property
    def num_modes(self) -> int:
        return self._data[6]

    @property
    def input_modes(self) -> list[int]:
        (flags,) = struct.unpack_from("<H", self._data, 7)
        return [n for n in range(16) if flags & (1 << n)]

    @property
    def output_modes(self) -> list[int]:
        (flags,) = struct.unpack_from("<H", self._data, 9)
        return [n for n in range(16) if flags & (1 << n)]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.capabilities)}, {repr(self.num_modes)}, {repr(self.input_modes)}, {repr(self.output_modes)})"


class PortInfoCombosMessage(AbstractPortInfoMessage):
    def __init__(
        self,
        port: PortID,
        combos: list[list[int]],
    ) -> None:
        super().__init__(5 + len(combos) * 2, port, InfoKind.COMBOS)

        flags = []
        for combo in combos:
            f = 0

            for mode in combo:
                f |= 1 << mode

            flags.append(f)

        struct.pack_into(f"<{len(flags)}H", self._data, 5, *flags)

    @property
    def combos(self) -> list[list[int]]:
        count = (len(self._data) - 5) // 2
        return [
            [m for m in range(16) if flags & (1 << m)]
            for flags in struct.unpack_from(f"<{count}H", self._data, 5)
        ]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.combos)})"


class AbstractPortModeInfoMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(
        self, length: int, port: PortID, mode: int, info_kind: ModeInfoKind
    ) -> None:
        super().__init__(length, MessageKind.PORT_MODE_INFO)

        self._data[3] = port
        self._data[4] = mode
        self._data[5] = info_kind

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def mode(self) -> int:
        return self._data[4]

    @property
    def info_kind(self) -> ModeInfoKind:
        return ModeInfoKind(self._data[5])


class PortModeInfoNameMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, name: str) -> None:
        name = name.encode("ascii")

        if len(name) == 0:
            raise ValueError("name cannot be empty")

        if len(name) > 11:
            raise ValueError("name must be 11 bytes or less")

        super().__init__(6 + len(name), port, mode, ModeInfoKind.NAME)

        self._data[6:] = name

    @property
    def name(self) -> str:
        return self._data[6:].decode("ascii").strip("\0")

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.name)})"


class PortModeInfoRawMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, min: float, max: float) -> None:
        super().__init__(14, port, mode, ModeInfoKind.RAW)

        struct.pack_into("<ff", self._data, 6, min, max)

    @property
    def min(self) -> float:
        return struct.unpack_from("<f", self._data, 6)[0]

    @property
    def max(self) -> float:
        return struct.unpack_from("<f", self._data, 10)[0]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.min)}, {repr(self.max)})"


class PortModeInfoPercentMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, min: float, max: float) -> None:
        super().__init__(14, port, mode, ModeInfoKind.PCT)

        struct.pack_into("<ff", self._data, 6, min, max)

    @property
    def min(self) -> float:
        return struct.unpack_from("<f", self._data, 6)[0]

    @property
    def max(self) -> float:
        return struct.unpack_from("<f", self._data, 10)[0]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.min)}, {repr(self.max)})"


class PortModeInfoSIMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, min: float, max: float) -> None:
        super().__init__(14, port, mode, ModeInfoKind.SI)

        struct.pack_into("<ff", self._data, 6, min, max)

    @property
    def min(self) -> float:
        return struct.unpack_from("<f", self._data, 6)[0]

    @property
    def max(self) -> float:
        return struct.unpack_from("<f", self._data, 10)[0]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.min)}, {repr(self.max)})"


class PortModeInfoSymbolMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, symbol: str) -> None:
        symbol = symbol.encode("ascii")

        if len(symbol) > 5:
            raise ValueError("symbol must be 5 bytes or less")

        super().__init__(6 + len(symbol), port, mode, ModeInfoKind.SYMBOL)

        self._data[6:] = symbol

    @property
    def symbol(self) -> str:
        return self._data[6:].decode("ascii").strip("\0")

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.symbol)})"


class PortModeInfoMappingMessage(AbstractPortModeInfoMessage):
    def __init__(
        self,
        port: PortID,
        mode: int,
        input_mapping: IODeviceMapping,
        output_mapping: IODeviceMapping,
    ) -> None:
        super().__init__(8, port, mode, ModeInfoKind.MAPPING)

        self._data[6] = input_mapping
        self._data[7] = output_mapping

    @property
    def input_mapping(self) -> IODeviceMapping:
        return IODeviceMapping(self._data[6])

    @property
    def output_mapping(self) -> IODeviceMapping:
        return IODeviceMapping(self._data[7])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.input_mapping)}, {repr(self.output_mapping)})"


class PortModeInfoMotorBiasMessage(AbstractPortModeInfoMessage):
    def __init__(self, port: PortID, mode: int, bias: int) -> None:
        super().__init__(7, port, mode, ModeInfoKind.MOTOR_BIAS)

        self._data[6] = bias

    @property
    def bias(self) -> int:
        return self._data[6]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.bias)})"


class PortModeInfoCapabilitiesMessage(AbstractPortModeInfoMessage):
    def __init__(
        self, port: PortID, mode: int, capabilities: IODeviceCapabilities
    ) -> None:
        super().__init__(12, port, mode, ModeInfoKind.CAPABILITIES)

        self._data[6:] = capabilities.to_bytes(6, "little")

    @property
    def capabilities(self) -> IODeviceCapabilities:
        return IODeviceCapabilities.from_bytes(self._data[6:], "little")

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.capabilities)})"


class PortModeInfoFormatMessage(AbstractPortModeInfoMessage):
    def __init__(
        self,
        port: PortID,
        mode: int,
        datasets: int,
        format: DataFormat,
        figures: int,
        decimals: int,
    ) -> None:
        super().__init__(10, port, mode, ModeInfoKind.FORMAT)

        self._data[6] = datasets
        self._data[7] = format
        self._data[8] = figures
        self._data[9] = decimals

    @property
    def datasets(self) -> int:
        return self._data[6]

    @property
    def format(self) -> DataFormat:
        return DataFormat(self._data[7])

    @property
    def figures(self) -> int:
        return self._data[8]

    @property
    def decimals(self) -> int:
        return self._data[9]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.datasets)}, {repr(self.format)}, {repr(self.figures)}, {repr(self.decimals)})"


class PortValueMessage(AbstractMessage):
    def __init__(self, port: PortID, fmt: str, *values: int | float) -> None:
        super().__init__(4 + struct.calcsize(fmt), MessageKind.PORT_VALUE)

        self._data[3] = port
        struct.pack_into(fmt, self._data, 4, *values)

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    def unpack(self, fmt: str) -> tuple[int | float, ...]:
        return struct.unpack_from(fmt, self._data, 4)

    def __repr__(self) -> str:
        fmt = f"<{self._data[0] - 4}b"
        values = ", ".join(repr(x) for x in struct.unpack_from(fmt, self._data, 4))
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(fmt)}, {values})"


class PortValueComboMessage(AbstractMessage):
    def __init__(
        self, port: PortID, modes: list[int], fmt: str, *values: int | float
    ) -> None:
        super().__init__(6 + struct.calcsize(fmt), MessageKind.PORT_VALUE_COMBO)

        mode_flags = 0
        for m in modes:
            mode_flags |= 1 << m

        self._data[3] = port
        struct.pack_into("<H", self._data, 4, mode_flags)
        struct.pack_into(fmt, self._data, 6, *values)

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def modes(self) -> list[int]:
        (flags,) = struct.unpack_from("<H", self._data, 4)
        return [m for m in range(16) if flags & (1 << m)]

    def unpack(self, fmt: str) -> tuple[int | float, ...]:
        return struct.unpack_from(fmt, self._data, 6)

    def __repr__(self) -> str:
        fmt = f"<{self._data[0] - 6}b"
        values = ", ".join(repr(x) for x in struct.unpack_from(fmt, self._data, 6))
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.modes)}, {repr(fmt)}, {values})"


class PortInputFormatMessage(AbstractMessage):
    def __init__(self, port: PortID, mode: int, delta: int, notify: bool) -> None:
        super().__init__(10, MessageKind.PORT_INPUT_FMT)

        struct.pack_into("<BBI?", self._data, 3, port, mode, delta, notify)

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def mode(self) -> int:
        return self._data[4]

    @property
    def delta(self) -> int:
        return struct.unpack_from("<I", self._data, 5)[0]

    @property
    def notify(self) -> bool:
        return bool(self._data[9])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.mode)}, {repr(self.delta)}, {repr(self.notify)})"


class PortInputFormatComboMessage(AbstractMessage):
    def __init__(
        self,
        port: PortID,
        combo: int,
        multi_update: bool,
        modes_and_datasets: list[int],
    ) -> None:
        super().__init__(7, MessageKind.PORT_INPUT_FMT_COMBO)

        combo &= 0xF
        if multi_update:
            combo |= 0x80

        modes_and_datasets_flags = 0
        for m in modes_and_datasets:
            modes_and_datasets_flags |= 1 << m

        struct.pack_into("<BBH", self._data, 3, port, combo, modes_and_datasets_flags)

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def combo(self) -> int:
        return self._data[4] & 0xF

    @property
    def multi_update(self) -> bool:
        return bool(self._data[4] & 0x80)

    @property
    def modes_and_datasets(self) -> list[int]:
        (flags,) = struct.unpack_from("<H", self._data, 5)
        return [m for m in range(16) if flags & (1 << m)]

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.combo)}, {repr(self.multi_update)}, {repr(self.modes_and_datasets)})"


###############################################################################
# Virtual port messages
###############################################################################


class AbstractVirtualPortSetupMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(self, length: int, command: VirtualPortSetupCommand) -> None:
        super().__init__(length, MessageKind.VIRTUAL_PORT_SETUP)

        self._data[3] = command

    @property
    def command(self) -> VirtualPortSetupCommand:
        return VirtualPortSetupCommand(self._data[3])


class VirtualPortSetupDisconnectMessage(AbstractVirtualPortSetupMessage):
    def __init__(self, port: PortID) -> None:
        super().__init__(5, VirtualPortSetupCommand.DISCONNECT)

        self._data[4] = port

    @property
    def port(self) -> PortID:
        return PortID(self._data[4])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port)})"


class VirtualPortSetupConnectMessage(AbstractVirtualPortSetupMessage):
    def __init__(self, port_a: PortID, port_b: PortID) -> None:
        super().__init__(6, VirtualPortSetupCommand.CONNECT)

        self._data[4] = port_a
        self._data[5] = port_b

    @property
    def port_a(self) -> PortID:
        return PortID(self._data[4])

    @property
    def port_b(self) -> PortID:
        return PortID(self._data[5])

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port_a)}, {repr(self.port_b)})"


###############################################################################
# Port output messages
###############################################################################


class AbstractPortOutputCommandMessage(AbstractMessage):
    @abc.abstractmethod
    def __init__(
        self,
        length: int,
        port: PortID,
        start: StartInfo,
        end: EndInfo,
        command: PortOutputCommand,
    ) -> None:
        super().__init__(length, MessageKind.PORT_OUTPUT_CMD)

        self._data[3] = port
        self._data[4] = start | end
        self._data[5] = command

    @property
    def port(self) -> PortID:
        return PortID(self._data[3])

    @property
    def start(self) -> StartInfo:
        return StartInfo(self._data[4] & 0xF0)

    @property
    def end(self) -> EndInfo:
        return EndInfo(self._data[4] & 0x0F)

    @property
    def command(self) -> PortOutputCommand:
        return PortOutputCommand(self._data[5])

    @abc.abstractmethod
    def __repr__(self, extra: str) -> str:
        return f"{self.__class__.__name__}({repr(self.port)}, {repr(self.start)}, {repr(self.end)}, {extra})"


class PortOutputCommandWriteDirectMessage(AbstractPortOutputCommandMessage):
    def __init__(
        self, port: PortID, start: StartInfo, end: EndInfo, payload: bytes
    ) -> None:
        super().__init__(
            6 + len(payload), port, start, end, PortOutputCommand.WRITE_DIRECT
        )

        if xor_bytes(payload) != 0x00:
            raise ValueError("payload has invalid checksum")

        self._data[6:] = payload

    @property
    def payload(self) -> bytes:
        return bytes(self._data[6:])

    def __repr__(self) -> str:
        return super().__repr__(repr(self.payload))


class PortOutputCommandWriteDirectModeDataMessage(AbstractPortOutputCommandMessage):
    def __init__(
        self,
        port: PortID,
        start: StartInfo,
        end: EndInfo,
        mode: int,
        fmt: str,
        *values: int | float,
    ) -> None:
        super().__init__(
            7 + struct.calcsize(fmt),
            port,
            start,
            end,
            PortOutputCommand.WRITE_DIRECT_MODE_DATA,
        )

        self._data[6] = mode
        struct.pack_into(fmt, self._data, 7, *values)

    @property
    def mode(self) -> int:
        return self._data[6]

    def unpack(self, fmt: str) -> tuple[int | float, ...]:
        return struct.unpack_from(fmt, self._data, 7)

    def __repr__(self) -> str:
        fmt = f"<{len(self._data) - 7}b"
        values = ", ".join(repr(d) for d in self.unpack(fmt))
        return super().__repr__(f"{repr(self.mode)}, {repr(fmt)}, {values}")


class PortOutputCommandFeedbackMessage(AbstractMessage):
    @overload
    def __init__(self, port: PortID, feedback: Feedback) -> None: ...

    @overload
    def __init__(
        self, port1: PortID, feedback1: Feedback, port2: PortID, feedback2: Feedback
    ) -> None: ...

    @overload
    def __init__(
        self,
        port1: PortID,
        feedback1: Feedback,
        port2: PortID,
        feedback2: Feedback,
        port3: PortID,
        feedback3: Feedback,
    ) -> None: ...

    def __init__(
        self,
        port1: PortID,
        feedback1: Feedback,
        port2: PortID | None = None,
        feedback2: Feedback | None = None,
        port3: PortID | None = None,
        feedback3: Feedback | None = None,
    ) -> None:
        length = 5

        if port2 is not None:
            length += 2

        if port3 is not None:
            length += 2

        super().__init__(length, MessageKind.PORT_OUTPUT_CMD_FEEDBACK)

        self._data[3] = port1
        self._data[4] = feedback1

        if port2 is not None:
            self._data[5] = port2
            self._data[6] = feedback2

        if port3 is not None:
            self._data[7] = port3
            self._data[8] = feedback3

    @property
    def port1(self) -> PortID:
        return PortID(self._data[3])

    @property
    def feedback1(self) -> Feedback:
        return Feedback(self._data[4])

    @property
    def port2(self) -> PortID:
        try:
            return PortID(self._data[5])
        except IndexError:
            return None

    @property
    def feedback2(self) -> Feedback:
        try:
            return Feedback(self._data[6])
        except IndexError:
            return None

    @property
    def port3(self) -> PortID:
        try:
            return PortID(self._data[7])
        except IndexError:
            return None

    @property
    def feedback3(self) -> Feedback:
        try:
            return Feedback(self._data[8])
        except IndexError:
            return None

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({repr(self.port1)}, {repr(self.feedback1)}, {repr(self.port2)}, {repr(self.feedback2)}, {repr(self.port3)}, {repr(self.feedback3)})"


###############################################################################
# Message parsing
###############################################################################


class _Lookup(NamedTuple):
    """Type descriminator."""

    index: int
    """The index of the bytecode that determines the type."""

    value: dict[IntEnum, type[AbstractMessage]] | dict[IntEnum, "_Lookup"]
    """
    A dictionary mapping a bytecode to the cooresponding Python type if the type can be determined or
    a dictionary mapping a bytecode to another lookup if more discrimination is required.
    """


# type descriminator for hub property message types
_HUB_PROPERTY_OP_CLASS_MAP = {
    HubPropertyOperation.SET: HubPropertySet,
    HubPropertyOperation.ENABLE_UPDATES: HubPropertyEnableUpdates,
    HubPropertyOperation.DISABLE_UPDATES: HubPropertyDisableUpdates,
    HubPropertyOperation.RESET: HubPropertyReset,
    HubPropertyOperation.REQUEST_UPDATE: HubPropertyRequestUpdate,
    HubPropertyOperation.UPDATE: HubPropertyUpdate,
}

# type descriminator for hub alert message types
_HUB_ALERT_OP_CLASS_MAP = {
    AlertOperation.ENABLE_UPDATES: HubAlertEnableUpdatesMessage,
    AlertOperation.DISABLE_UPDATES: HubAlertDisableUpdatesMessage,
    AlertOperation.REQUEST_UPDATE: HubAlertRequestUpdateMessage,
    AlertOperation.UPDATE: HubAlertUpdateMessage,
}

# type descriminator for hub attached I/O message types
_HUB_ATTACHED_IO_EVENT_CLASS_MAP = {
    IOEvent.DETACHED: HubIODetachedMessage,
    IOEvent.ATTACHED: HubIOAttachedMessage,
    IOEvent.ATTACHED_VIRTUAL: HubIOAttachedVirtualMessage,
}

# type descriminator for hardware network command message types
_HW_NET_CMD_CLASS_MAP = {
    HwNetCmd.CONNECTION_REQUEST: HwNetCmdRequestConnectionMessage,
    HwNetCmd.FAMILY_REQUEST: HwNetCmdRequestFamilyMessage,
    HwNetCmd.FAMILY_SET: HwNetCmdSetFamilyMessage,
    HwNetCmd.JOIN_DENIED: HwNetCmdJoinDeniedMessage,
    HwNetCmd.GET_FAMILY: HwNetCmdGetFamilyMessage,
    HwNetCmd.FAMILY: HwNetCmdFamilyMessage,
    HwNetCmd.GET_SUBFAMILY: HwNetCmdGetSubfamilyMessage,
    HwNetCmd.SUBFAMILY: HwNetCmdSubfamilyMessage,
    HwNetCmd.SUBFAMILY_SET: HwNetCmdSetSubfamilyMessage,
    HwNetCmd.GET_EXTENDED_FAMILY: HwNetCmdGetExtendedFamilyMessage,
    HwNetCmd.EXTENDED_FAMILY: HwNetCmdExtendedFamilyMessage,
    HwNetCmd.EXTENDED_FAMILY_SET: HwNetCmdSetExtendedFamilyMessage,
    HwNetCmd.RESET_LONG_PRESS: HwNetCmdResetLongPressMessage,
}

_PORT_INPUT_FMT_SETUP_COMBO_CLASS_MAP = {
    PortInfoFormatSetupCommand.SET: PortFormatSetupComboMessage,
    PortInfoFormatSetupCommand.LOCK: PortFormatSetupComboLockMessage,
    PortInfoFormatSetupCommand.UNLOCK_ENABLED: PortFormatSetupComboUnlockEnabledMessage,
    PortInfoFormatSetupCommand.UNLOCK_DISABLED: PortFormatSetupComboUnlockDisabledMessage,
    PortInfoFormatSetupCommand.RESET: PortFormatSetupComboResetMessage,
}

_PORT_INFO_CLASS_MAP = {
    InfoKind.MODE_INFO: PortInfoModeInfoMessage,
    InfoKind.COMBOS: PortInfoCombosMessage,
}

_PORT_MODE_INFO_CLASS_MAP = {
    ModeInfoKind.NAME: PortModeInfoNameMessage,
    ModeInfoKind.RAW: PortModeInfoRawMessage,
    ModeInfoKind.PCT: PortModeInfoPercentMessage,
    ModeInfoKind.SI: PortModeInfoSIMessage,
    ModeInfoKind.SYMBOL: PortModeInfoSymbolMessage,
    ModeInfoKind.MAPPING: PortModeInfoMappingMessage,
    ModeInfoKind.MOTOR_BIAS: PortModeInfoMotorBiasMessage,
    ModeInfoKind.CAPABILITIES: PortModeInfoCapabilitiesMessage,
    ModeInfoKind.FORMAT: PortModeInfoFormatMessage,
}

_VIRTUAL_PORT_SETUP_CLASS_MAP = {
    VirtualPortSetupCommand.DISCONNECT: VirtualPortSetupDisconnectMessage,
    VirtualPortSetupCommand.CONNECT: VirtualPortSetupConnectMessage,
}

_OUTPUT_CMD_CLASS_MAP = {
    PortOutputCommand.WRITE_DIRECT: PortOutputCommandWriteDirectMessage,
    PortOutputCommand.WRITE_DIRECT_MODE_DATA: PortOutputCommandWriteDirectModeDataMessage,
}

# base type descriminator for messages
_MESSAGE_CLASS_MAP = {
    MessageKind.HUB_PROPERTY: _Lookup(4, _HUB_PROPERTY_OP_CLASS_MAP),
    MessageKind.HUB_ACTION: HubActionMessage,
    MessageKind.HUB_ALERT: _Lookup(4, _HUB_ALERT_OP_CLASS_MAP),
    MessageKind.HUB_ATTACHED_IO: _Lookup(4, _HUB_ATTACHED_IO_EVENT_CLASS_MAP),
    MessageKind.ERROR: ErrorMessage,
    MessageKind.HW_NET_CMD: _Lookup(3, _HW_NET_CMD_CLASS_MAP),
    MessageKind.FW_UPDATE: FirmwareUpdateMessage,
    MessageKind.PORT_INFO_REQ: PortInfoRequestMessage,
    MessageKind.PORT_MODE_INFO_REQ: PortModeInfoRequestMessage,
    MessageKind.PORT_INPUT_FMT_SETUP: PortInputFormatSetupMessage,
    MessageKind.PORT_INPUT_FMT_SETUP_COMBO: _Lookup(
        4, _PORT_INPUT_FMT_SETUP_COMBO_CLASS_MAP
    ),
    MessageKind.PORT_INFO: _Lookup(4, _PORT_INFO_CLASS_MAP),
    MessageKind.PORT_MODE_INFO: _Lookup(5, _PORT_MODE_INFO_CLASS_MAP),
    MessageKind.PORT_VALUE: PortValueMessage,
    MessageKind.PORT_VALUE_COMBO: PortValueComboMessage,
    MessageKind.PORT_INPUT_FMT: PortInputFormatMessage,
    MessageKind.PORT_INPUT_FMT_COMBO: PortInputFormatComboMessage,
    MessageKind.VIRTUAL_PORT_SETUP: _Lookup(3, _VIRTUAL_PORT_SETUP_CLASS_MAP),
    MessageKind.PORT_OUTPUT_CMD: _Lookup(5, _OUTPUT_CMD_CLASS_MAP),
    MessageKind.PORT_OUTPUT_CMD_FEEDBACK: PortOutputCommandFeedbackMessage,
}


def parse_message(data: bytes) -> AbstractMessage:
    """
    Parses ``data`` and returns a message object.

    Args:
        data: Raw binary data of the message.

    Returns:
        A new message object whose type corresponds to the message data.
    """
    cls = _Lookup(2, _MESSAGE_CLASS_MAP)
    while isinstance(cls, _Lookup):
        kind = data[cls.index]
        cls = cls.value[kind]

    # bypass __init__() since we already have the encoded data
    msg = object.__new__(cls)
    msg._data = data

    return msg
