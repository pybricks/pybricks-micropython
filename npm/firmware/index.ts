// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

import JSZip, { JSZipObject } from 'jszip';
import { PACKAGE_VERSION } from './version';

const encoder = new TextEncoder();

/**
 * String containing the firmware version.
 */
export const firmwareVersion = PACKAGE_VERSION.substring(
    PACKAGE_VERSION.lastIndexOf('v') + 1
);

/**
 * LEGO Powered Up Hub IDs
 */
export enum HubType {
    /**
     * The LEGO BOOST Move hub firmware file.
     */
    MoveHub = 0x40,

    /**
     * The LEGO Powered Up 2-port hub firmware file.
     */
    CityHub = 0x41,

    /**
     * The LEGO Technic 4-port hub firmware file.
     */
    TechnicHub = 0x80,

    /**
     * The LEGO Technic Large hub (SPIKE Prime and MINDSTORMS Robot Inventor) firmware file.
     */
    PrimeHub = 0x81,

    /**
     * The LEGO Technic Small hub (SPIKE Essential) firmware file.
     */
    EssentialHub = 0x83,
}

/**
 * Map of hub type to firmware file name.
 */
export const zipFileNameMap: ReadonlyMap<HubType, string> = new Map([
    [HubType.MoveHub, 'movehub.zip'],
    [HubType.CityHub, 'cityhub.zip'],
    [HubType.TechnicHub, 'technichub.zip'],
    [HubType.PrimeHub, 'primehub.zip'],
    [HubType.EssentialHub, 'essentialhub.zip'],
]);

/**
 * Firmware metadata v1.0.0 properties.
 */
export type FirmwareMetadataV100 = {
    /** The version of the metadata itself. */
    'metadata-version': '1.0.0';
    /** The version of the firmware binary. */
    'firmware-version': string;
    /** The type of hub the firmware runs on. */
    'device-id': HubType;
    /** The type of checksum used by the device bootloader to verify the firmware. */
    'checksum-type': 'sum' | 'crc32';
    /** The .mpy file ABI version compatible with the firmware binary. */
    'mpy-abi-version': number;
    /** Command line options that need to be passed to mpy-cross to produce a compatible .mpy file. */
    'mpy-cross-options': string[];
    /** The offset from the start of the firmware where it expects to find a .mpy file. */
    'user-mpy-offset': number;
    /** The maximum firmware size allowed on the hub. */
    'max-firmware-size': number;
};

/**
 * Firmware metadata v1.1.0 properties.
 */
export type FirmwareMetadataV110 = Omit<
    FirmwareMetadataV100,
    'metadata-version'
> & {
    /** The version of the metadata itself. */
    'metadata-version': '1.1.0';
    /** The offset to where the hub name is stored in the firmware. */
    'hub-name-offset': number;
    /** The maximum size of the firmware name in bytes, including the zero-termination. */
    'max-hub-name-size': number;
};

/**
 * Firmware metadata v2.0.0 properties.
 */
export type FirmwareMetadataV200 = {
    /** The version of the metadata itself. */
    'metadata-version': '2.0.0';
    /** The version of the firmware binary. */
    'firmware-version': string;
    /** The type of hub the firmware runs on. */
    'device-id': HubType;
    /** The type of checksum used by the device bootloader to verify the firmware. */
    'checksum-type': 'sum' | 'crc32';
    /** The data size for the checksum calculation. */
    'checksum-size': number;
    /** The offset to where the hub name is stored in the firmware. */
    'hub-name-offset': number;
    /** The maximum size of the firmware name in bytes, including the zero-termination. */
    'hub-name-size': number;
};

/** Firmware metadata of any version. */
export type FirmwareMetadata =
    | FirmwareMetadataV100
    | FirmwareMetadataV110
    | FirmwareMetadataV200;

/** Types of errors that can be raised by FirmwareReader. */
export enum FirmwareReaderErrorCode {
    /** There was a problem with the zip file data. */
    ZipError,
    /** The zip file is missing the firmware-base.bin file. */
    MissingFirmwareBaseBin,
    /** The zip file is missing the firmware.metadata.json file. */
    MissingMetadataJson,
    /** The zip file is missing the ReadMe_OSS.txt file. */
    MissingReadmeOssTxt,
}

/** Maps error codes to error messages. */
const firmwareReaderErrorMessage: ReadonlyMap<FirmwareReaderErrorCode, string> =
    new Map([
        [FirmwareReaderErrorCode.ZipError, 'bad zip data'],
        [
            FirmwareReaderErrorCode.MissingFirmwareBaseBin,
            'missing firmware-base.bin',
        ],
        [
            FirmwareReaderErrorCode.MissingMetadataJson,
            'missing firmware.metadata.json',
        ],
        [FirmwareReaderErrorCode.MissingReadmeOssTxt, 'ReadMe_OSS.txt'],
    ]);

/** Errors throw by FirmwareReader */
export class FirmwareReaderError extends Error {
    /**
     * Creates a new error.
     * @param code The error code.
     */
    public constructor(
        public readonly code: FirmwareReaderErrorCode,
        public readonly cause?: Error
    ) {
        super(firmwareReaderErrorMessage.get(code));
        Object.setPrototypeOf(this, FirmwareReaderError.prototype);
        this.name = 'FirmwareReaderError';
    }
}

function isError(err: unknown): err is Error {
    const maybeError = err as Error;

    return (
        maybeError !== undefined &&
        typeof maybeError.name === 'string' &&
        typeof maybeError.message === 'string'
    );
}

function ensureError(err: unknown): Error {
    if (isError(err)) {
        return err;
    }

    if (typeof err === 'string') {
        return new Error(err);
    }

    return Error(String(err));
}

async function wrapError<T>(
    callback: () => Promise<T>,
    code: FirmwareReaderErrorCode
): Promise<T> {
    try {
        return await callback();
    } catch (err) {
        throw new FirmwareReaderError(code, ensureError(err));
    }
}

/**
 * Helper class to read firmware zip file and extract the contents.
 *
 * Loading is async, so call the static `load` method to create a new instance
 * instead of using the `new` keyword to call the constructor.
 */
export class FirmwareReader {
    private firmware?: JSZipObject | null;
    private metadata?: JSZipObject | null;
    private mainPy?: JSZipObject | null;
    private readMeOss?: JSZipObject | null;

    private constructor() {}

    /**
     * Loads data from a firmware.zip file and does a few sanity checks.
     * @param zipData The firmware.zip file binary data.
     */
    public static async load(
        zipData: Uint8Array | ArrayBuffer | Blob
    ): Promise<FirmwareReader> {
        const reader = new FirmwareReader();
        const zip = await wrapError(
            () => JSZip.loadAsync(zipData),
            FirmwareReaderErrorCode.ZipError
        );

        reader.firmware = zip.file('firmware-base.bin');
        if (!reader.firmware) {
            throw new FirmwareReaderError(
                FirmwareReaderErrorCode.MissingFirmwareBaseBin
            );
        }

        reader.metadata = zip.file('firmware.metadata.json');
        if (!reader.metadata) {
            throw new FirmwareReaderError(
                FirmwareReaderErrorCode.MissingMetadataJson
            );
        }

        // main.py is optional
        reader.mainPy = zip.file('main.py');

        reader.readMeOss = zip.file('ReadMe_OSS.txt');
        if (!reader.readMeOss) {
            throw new FirmwareReaderError(
                FirmwareReaderErrorCode.MissingReadmeOssTxt
            );
        }

        return reader;
    }

    /** Reads the firmware-base.bin file from the zip file. */
    public readFirmwareBase(): Promise<Uint8Array> {
        return this.firmware!.async('uint8array');
    }

    /** Reads the firmware.metadata.json file from the zip file. */
    public async readMetadata(): Promise<Readonly<FirmwareMetadata>> {
        return JSON.parse(await this.metadata!.async('text'));
    }

    /**
     * Reads the main.py file from the zip file.
     *
     * Returns `undefined` if the file does not exist.
     */
    public readMainPy(): Promise<string | undefined> {
        return this.mainPy?.async('text') ?? Promise.resolve(undefined);
    }

    /** Reads the ReadMe_OSS.txt file from the zip file. */
    public readReadMeOss(): Promise<string> {
        return this.readMeOss!.async('text');
    }
}

/**
 * Type discriminator for metadata v1.0.0.
 * @param metadata The metadata to test.
 * @returns True if the metadata is v1.0.0.
 */
export function metadataIsV100(
    metadata: FirmwareMetadata
): metadata is FirmwareMetadataV100 {
    return metadata['metadata-version'] === '1.0.0';
}

/**
 * Type discriminator for metadata v1.1.0.
 * @param metadata The metadata to test.
 * @returns True if the metadata is v1.1.0.
 */
export function metadataIsV110(
    metadata: FirmwareMetadata
): metadata is FirmwareMetadataV110 {
    return metadata['metadata-version'] === '1.1.0';
}

/**
 * Type discriminator for metadata v2.0.0.
 * @param metadata The metadata to test.
 * @returns True if the metadata is v2.0.0.
 */
export function metadataIsV200(
    metadata: FirmwareMetadata
): metadata is FirmwareMetadataV200 {
    return metadata['metadata-version'] === '2.0.0';
}

/**
 * Encodes a firmware name as UTF-8 bytes with zero-termination.
 *
 * If the name is too long to fit in the size specified by the metadata, the
 * name will be truncated. The resulting value can be written to the firmware
 * image at 'hub-name-offset'.
 *
 * @param name The hub name.
 * @param metadata The firmware metadata.
 */
export function encodeHubName(
    name: string,
    metadata: FirmwareMetadata
): Uint8Array {
    const nameSize = metadataIsV100(metadata)
        ? undefined
        : metadataIsV110(metadata)
        ? metadata['max-hub-name-size']
        : metadata['hub-name-size'];

    if (nameSize === undefined) {
        throw new Error('firmware image does not support firmware name');
    }

    const bytes = new Uint8Array(nameSize);

    // subarray ensures zero termination if encoded length is >= 'max-hub-name-size'.
    encoder.encodeInto(name, bytes.subarray(0, bytes.length - 1));

    return bytes;
}
