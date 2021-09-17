// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

import * as fs from 'fs';
import * as path from 'path';
import * as util from 'util';
import { firmwareVersion, FirmwareReader, FirmwareReaderErrorCode } from '.';

const readFile = util.promisify(fs.readFile);

test('firmware version', () => {
    expect(firmwareVersion).toMatch(/^\d+\.\d+\.\d+/);
});

test('bad zip data', async () => {
    try {
        await FirmwareReader.load(new Uint8Array(100));
        fail('FirmwareReader.load() should have failed');
    } catch (err: any) {
        expect(err.name).toMatch('FirmwareReaderError');
        expect(err.code).toBe(FirmwareReaderErrorCode.ZipError);
    }
});

test('missing firmware-base.bin', async () => {
    var file = await readFile(
        path.resolve(__dirname, '__tests__', 'movehub-no-firmware-base.zip')
    );
    try {
        await FirmwareReader.load(file);
        fail('FirmwareReader.load() should have failed');
    } catch (err: any) {
        expect(err.name).toMatch('FirmwareReaderError');
        expect(err.code).toBe(FirmwareReaderErrorCode.MissingFirmwareBaseBin);
    }
});

test('missing firmware.metadata.json', async () => {
    var file = await readFile(
        path.resolve(__dirname, '__tests__', 'movehub-no-metadata.zip')
    );
    try {
        await FirmwareReader.load(file);
        fail('FirmwareReader.load() should have failed');
    } catch (err: any) {
        expect(err.name).toMatch('FirmwareReaderError');
        expect(err.code).toBe(FirmwareReaderErrorCode.MissingMetadataJson);
    }
});

test('missing main.py', async () => {
    var file = await readFile(
        path.resolve(__dirname, '__tests__', 'movehub-no-main-py.zip')
    );
    try {
        await FirmwareReader.load(file);
        fail('FirmwareReader.load() should have failed');
    } catch (err: any) {
        expect(err.name).toMatch('FirmwareReaderError');
        expect(err.code).toBe(FirmwareReaderErrorCode.MissingMainPy);
    }
});

test('missing ReadMe_OSS.txt', async () => {
    var file = await readFile(
        path.resolve(__dirname, '__tests__', 'movehub-no-readme-oss.zip')
    );
    try {
        await FirmwareReader.load(file);
        fail('FirmwareReader.load() should have failed');
    } catch (err: any) {
        expect(err.name).toMatch('FirmwareReaderError');
        expect(err.code).toBe(FirmwareReaderErrorCode.MissingReadmeOssTxt);
    }
});

test('reading data works', async () => {
    var file = await readFile(
        path.resolve(__dirname, '__tests__', 'movehub.zip')
    );
    var reader = await FirmwareReader.load(file);
    expect(await reader.readFirmwareBase()).toMatchSnapshot();
    expect(await reader.readMetadata()).toMatchSnapshot();
    expect(await reader.readMainPy()).toMatchSnapshot();
    expect(await reader.readReadMeOss()).toMatchSnapshot();
});
