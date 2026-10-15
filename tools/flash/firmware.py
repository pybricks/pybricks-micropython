# SPDX-License-Identifier: MIT
# Copyright (c) 2022-2023 The Pybricks Authors

"""
Utilities for working with Pybricks ``firmware.zip`` files.
"""

import io
import json
import os
import struct
import zipfile
from typing import Any, BinaryIO, Literal, TypedDict, TypeGuard

import semver

from checksum import crc32_checksum, sum_complement
from lwp3.bytecodes import HubKind


class FirmwareMetadataV100(
    TypedDict(
        "V100",
        {
            "metadata-version": Literal["1.0.0"],
            "firmware-version": str,
            "device-id": Literal[0x40, 0x41, 0x80, 0x81],
            "checksum-type": Literal["sum", "crc32"],
            "mpy-abi-version": int,
            "mpy-cross-options": list[str],
            "user-mpy-offset": int,
            "max-firmware-size": int,
        },
    )
):
    """
    Type for data contained in v1.0.0 ``firmware.metadata.json`` files.
    """


class FirmwareMetadataV110(
    FirmwareMetadataV100,
    TypedDict(
        "V110",
        {
            # changed
            "metadata-version": Literal["1.1.0"],
            # added
            "hub-name-offset": int,
            "max-hub-name-size": int,
        },
    ),
):
    """
    Type for data contained in v1.1.0 ``firmware.metadata.json`` files.
    """


class FirmwareMetadataV200(
    TypedDict(
        "V200",
        {
            "metadata-version": Literal["2.0.0"],
            "firmware-version": str,
            "device-id": Literal[0x40, 0x41, 0x80, 0x81, 0x83],
            "checksum-type": Literal["sum", "crc32"],
            "checksum-size": int,
            "hub-name-offset": int,
            "hub-name-size": int,
        },
    )
):
    """
    Type for data contained in v2.0.0 ``firmware.metadata.json`` files.
    """


class FirmwareMetadataV210(
    FirmwareMetadataV200,
    TypedDict(
        "V210",
        {
            # changed
            "metadata-version": Literal["2.1.0"],
            "device-id": Literal[0x40, 0x41, 0x80, 0x81, 0x83, 0xE0, 0xE1, 0xE2],
            "checksum-type": Literal["sum", "crc32", "none"],
        },
    ),
):
    """
    Type for data contained in v2.1.0 ``firmware.metadata.json`` files.
    """


AnyFirmwareV1Metadata = FirmwareMetadataV100 | FirmwareMetadataV110
"""
Type for data contained in ``firmware.metadata.json`` files of any 1.x version.
"""

AnyFirmwareV2Metadata = FirmwareMetadataV200 | FirmwareMetadataV210
"""
Type for data contained in ``firmware.metadata.json`` files of any 2.x version.
"""

AnyFirmwareMetadata = AnyFirmwareV1Metadata | AnyFirmwareV2Metadata
"""
Type for data contained in ``firmware.metadata.json`` files of any 1.x or 2.x version.
"""


def _firmware_metadata_is_v1(
    metadata: AnyFirmwareMetadata,
) -> TypeGuard[AnyFirmwareV1Metadata]:
    return metadata["metadata-version"].startswith("1.")


def _firmware_metadata_is_v2(
    metadata: AnyFirmwareMetadata,
) -> TypeGuard[AnyFirmwareV2Metadata]:
    return metadata["metadata-version"].startswith("2.")


def _create_firmware_v1(
    metadata: AnyFirmwareV1Metadata, archive: zipfile.ZipFile, name: str | None
) -> bytearray:
    base = archive.open("firmware-base.bin").read()

    if "main.py" in archive.namelist():
        print("Warning: main.py is no longer supported and will be ignored.")
    mpy = b""

    # start with base firmware binary blob
    firmware = bytearray(base)
    # pad with 0s until user-mpy-offset
    firmware.extend(0 for _ in range(metadata["user-mpy-offset"] - len(firmware)))
    # append 32-bit little-endian main.mpy file size
    firmware.extend(struct.pack("<I", len(mpy)))
    # append main.mpy file
    firmware.extend(mpy)
    # pad with 0s to align to 4-byte boundary
    firmware.extend(0 for _ in range(-len(firmware) % 4))

    # Update hub name if given
    if name:
        if semver.compare(metadata["metadata-version"], "1.1.0") < 0:
            raise ValueError(
                "this firmware image does not support setting the hub name"
            )

        name = name.encode() + b"\0"

        max_size = metadata["max-hub-name-size"]

        if len(name) > max_size:
            raise ValueError(
                f"name is too big - must be < {metadata['max-hub-name-size']} UTF-8 bytes"
            )

        offset = metadata["hub-name-offset"]
        firmware[offset : offset + len(name)] = name

    # Get checksum for this firmware
    if metadata["checksum-type"] == "sum":
        checksum = sum_complement(io.BytesIO(firmware), metadata["max-firmware-size"])
    elif metadata["checksum-type"] == "crc32":
        checksum = crc32_checksum(io.BytesIO(firmware), metadata["max-firmware-size"])
    else:
        raise ValueError(f"unsupported checksum type: {metadata['checksum-type']}")

    # Append checksum to the firmware
    firmware.extend(struct.pack("<I", checksum))

    return firmware


def _create_firmware_v2(
    metadata: AnyFirmwareV2Metadata, archive: zipfile.ZipFile, name: str | None
) -> bytearray:
    base = archive.open("firmware-base.bin").read()

    # start with base firmware binary blob
    firmware = bytearray(base)

    # Update hub name if given
    if name:
        if not metadata["hub-name-offset"]:
            raise ValueError("this firmware does not support changing the hub name")

        name = name.encode() + b"\0"

        max_size = metadata["hub-name-size"]

        if len(name) > max_size:
            raise ValueError(
                f"name is too big - must be < {metadata['hub-name-size']} UTF-8 bytes"
            )

        offset = metadata["hub-name-offset"]
        firmware[offset : offset + len(name)] = name

    # Get checksum for this firmware
    if metadata["checksum-type"] == "sum":
        checksum = sum_complement(io.BytesIO(firmware), metadata["checksum-size"])
    elif metadata["checksum-type"] == "crc32":
        checksum = crc32_checksum(io.BytesIO(firmware), metadata["checksum-size"])
    elif (
        semver.compare(metadata["metadata-version"], "2.1.0") >= 0
        and metadata["checksum-type"] == "none"
    ):
        return firmware
    else:
        raise ValueError(f"unsupported checksum type: {metadata['checksum-type']}")

    # Append checksum to the firmware
    firmware.extend(struct.pack("<I", checksum))

    return firmware


def _create_firmware_variant_v3(
    variant: dict[str, Any], archive: zipfile.ZipFile, name: str | None
) -> bytearray:
    base = archive.open(variant["firmware"]).read()

    # start with base firmware binary blob
    firmware = bytearray(base)

    # Update hub name if given and renaming is supported.
    if name and variant["hub-name-offset"]:
        encoded_name = name.encode() + b"\0"
        max_size = variant["hub-name-size"]
        if len(encoded_name) > max_size:
            raise ValueError(
                f"name is too big - must be < {variant['hub-name-size']} UTF-8 bytes"
            )
        offset = variant["hub-name-offset"]
        firmware[offset : offset + len(encoded_name)] = encoded_name

    # Get checksum for this firmware
    if variant["checksum-type"] == "sum":
        checksum = sum_complement(io.BytesIO(firmware), variant["checksum-size"])
    elif variant["checksum-type"] == "crc32":
        checksum = crc32_checksum(io.BytesIO(firmware), variant["checksum-size"])
    elif variant["checksum-type"] == "none":
        return firmware
    else:
        raise ValueError(f"unsupported checksum type: {variant['checksum-type']}")

    # Append checksum to the firmware
    firmware.extend(struct.pack("<I", checksum))

    return firmware


def create_firmware_blob(
    firmware_zip: str | os.PathLike | BinaryIO, name: str | None = None
) -> tuple[HubKind, dict[str, bytes] | bytes]:
    """Creates firmware blobs from base firmware and an optional custom name.

    Arguments:
        firmware_zip:
            Path to the firmware zip file or a file-like object.
        name:
            A custom name for the hub.

    Returns:
        Tuple of the hub kind and the firmware blob(s) for flashing. For v3.x
        archives this is a dict mapping each platform name to its composite
        binary blob. For older v1.x and v2.x archives this is a single
        composite binary blob.

    Raises:
        ValueError:
            A name is given but the firmware does not support it or the name
            exceeds the alloted space in the firmware.

    """

    with zipfile.ZipFile(firmware_zip) as archive:
        with archive.open("firmware.metadata.json") as f:
            metadata: dict[str, Any] = json.load(f)

        version = metadata["metadata-version"]
        hub_kind = HubKind(metadata["device-id"])

        # Older metadata did not specify a platform name, but they mapped
        # directly to a single firmware binary blob from the hub type.
        legacy_pbio_platform = {
            HubKind.BOOST: "move_hub",
            HubKind.CITY: "city_hub",
            HubKind.TECHNIC: "technic_hub",
            HubKind.TECHNIC_LARGE: "prime_hub",
            HubKind.TECHNIC_SMALL: "essential_hub",
        }.get(hub_kind)

        if _firmware_metadata_is_v1(metadata):
            return hub_kind, {
                legacy_pbio_platform: bytes(
                    _create_firmware_v1(metadata, archive, name)
                )
            }

        if _firmware_metadata_is_v2(metadata):
            return hub_kind, {
                legacy_pbio_platform: bytes(
                    _create_firmware_v2(metadata, archive, name)
                )
            }

        if not version.startswith("3."):
            raise ValueError(f"unsupported metadata version: {version}")

        firmwares = {
            variant["platform"]: _create_firmware_variant_v3(variant, archive, name)
            for variant in metadata["variants"]
        }

        return hub_kind, firmwares
