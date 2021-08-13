// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

import JSZip, { JSZipObject } from 'jszip';
import {PACKAGE_VERSION} from './version';

/**
 * String containing the firmware version.
 */
export const firmwareVersion = PACKAGE_VERSION.substring(PACKAGE_VERSION.lastIndexOf('v') + 1);

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
}

/**
 * Map of hub type to firmware file name.
 */
export const zipFileNameMap: ReadonlyMap<HubType, string> = new Map([
    [HubType.MoveHub, 'movehub.zip'],
    [HubType.CityHub, 'cityhub.zip'],
    [HubType.TechnicHub, 'technichub.zip'],
]);

/**
 * Firmware metadata properties.
 */
export interface FirmwareMetadata {
    /** The version of the metadata itself. */
    'metadata-version': string;
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
}

/** Types of errors that can be raised by FirmwareReader. */
export enum FirmwareReaderErrorCode {
    /** There was a problem with the zip file data. */
    ZipError,
    /** The zip file is missing the firmware-base.bin file. */
    MissingFirmwareBaseBin,
    /** The zip file is missing the firmware.metadata.json file. */
    MissingMetadataJson,
    /** The zip file is missing the main.py file. */
    MissingMainPy,
    /** The zip file is missing the ReadMe_OSS.txt file. */
    MissingReadmeOssTxt,
}

/** Maps error codes to error messages. */
const firmwareReaderErrorMessage: ReadonlyMap<
    FirmwareReaderErrorCode,
    string
> = new Map([
    [FirmwareReaderErrorCode.ZipError, 'bad zip data'],
    [
        FirmwareReaderErrorCode.MissingFirmwareBaseBin,
        'missing firmware-base.bin',
    ],
    [
        FirmwareReaderErrorCode.MissingMetadataJson,
        'missing firmware.metadata.json',
    ],
    [FirmwareReaderErrorCode.MissingMainPy, 'missing main.py'],
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

async function wrapError<T>(
    callback: () => Promise<T>,
    code: FirmwareReaderErrorCode
): Promise<T> {
    try {
        return await callback();
    } catch (err) {
        throw new FirmwareReaderError(code, err);
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
    public static async load(zipData: Uint8Array | ArrayBuffer | Blob): Promise<FirmwareReader> {
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

        reader.mainPy = zip.file('main.py');
        if (!reader.mainPy) {
            throw new FirmwareReaderError(
                FirmwareReaderErrorCode.MissingMainPy
            );
        }

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

    /** Reads the main.py file from the zip file. */
    public readMainPy(): Promise<string> {
        return this.mainPy!.async('text');
    }

    /** Reads the ReadMe_OSS.txt file from the zip file. */
    public readReadMeOss(): Promise<string> {
        return this.readMeOss!.async('text');
    }
}
