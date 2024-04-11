## 7.17.0 - 2024-04-11

### Changed
- Updated firmware to v3.5.0.

## 7.16.0 - 2024-04-05

### Changed
- Updated firmware to v3.5.0b2.

## 7.15.0 - 2024-03-21

### Changed
- Updated firmware to v3.5.0b1.

## 7.14.0 - 2024-03-11

### Changed
- Updated firmware to v3.4.0.

## 7.13.0 - 2024-03-06

### Changed
- Updated firmware to v3.4.0b3.

## 7.12.0 - 2024-02-10

### Changed
- Updated firmware to v3.4.0b2.

## 7.11.0 - 2024-01-30

### Changed
- Updated firmware to v3.4.0b1.

## 7.10.0 - 2023-11-24

### Changed
- Updated firmware to v3.3.0.

## 7.9.0 - 2023-11-20

### Changed
- Updated firmware to v3.3.0c1.

## 7.8.0 - 2023-10-26

### Changed
- Updated firmware to v3.3.0b9.

## 7.7.0 - 2023-07-07

### Changed
- Updated firmware to v3.3.0b8.

## 7.6.0 - 2023-06-30

### Changed
- Updated firmware to v3.3.0b7.

## 7.5.0 - 2023-06-02

### Changed
- Updated firmware to v3.3.0b6.

## 7.4.0 - 2023-05-16

### Changed
- Updated firmware to v3.3.0b5.

## 7.3.1 - 2023-04-28

### Changed
- Rebuild with corrected firmware to v3.3.0b4.

## 7.3.0 - 2023-04-28

### Changed
- Updated firmware to v3.3.0b4.

## 7.2.0 - 2023-03-28

### Changed
- Updated firmware to v3.3.0b3.

## 7.1.2 - 2023-03-08

### Fixed
- Fixed wrong firmware version.

## 7.1.0 - 2023-03-08

### Changed
- Updated firmware to v3.3.0b2.

## 7.0.0 - 2023-02-17

### Added
- Added support for v2.1.0 metadata.

### Changed
- Updated firmware to v3.3.0b1.

## 6.7.0 - 2023-01-06

### Changed
- Updated firmware to v3.2.2.

## 6.6.0 - 2022-12-26

### Changed
- Updated firmware to v3.2.1.

## 6.5.0 - 2022-12-20

### Changed
- Updated firmware to v3.2.0.

## 6.4.0 - 2022-12-09

### Changed
- Updated firmware to v3.2.0c1.

## 6.3.0 - 2022-12-02

### Changed
- Updated firmware to v3.2.0b6.

## 6.2.0 - 2022-11-11

### Changed
- Updated firmware to v3.2.0b5.

## 6.0.1 - 2022-09-14
### Removed
- Removed invalid property on `FirmwareMetadataV110` type.

## 6.0.0 - 2022-09-14
### Added
- Added support for firmware.metadata.json v2.0.0.
- Added metadata version type discrimination functions.
### Changed
- Changed definition of `FirmwareMetadata` type (potentially breaking).

## 5.0.0 - 2022-07-20
### Changed
- Made `main.py` optional.

## 4.17.0 - 2022-07-20
### Added
- Added SPIKE Prime and Essential hubs.
### Changed
- Updated firmware to v3.2.0b3.

## 4.16.1 - 2022-07-03
### Changed
- Rebuild with correct firmware.

## 4.16.0 - 2022-07-03
### Changed
- Updated firmware to v3.2.0b2.

## 4.15.0 - 2022-06-03
### Changed
- Updated firmware to v3.2.0b1.

## 4.14.0 - 2021-12-16
### Changed
- Updated firmware to v3.1.0.

## 4.13.0 - 2021-11-19
### Changed
- Dropped fallback to default name in `encodeHubName()` on empty string.
- Use Typescript optional syntax in `FirmwareMetadata` instead of `| undefined`.

## 4.13.0-rc.1 - 2021-11-19
### Added
- Added support for v1.1.0 metadata.
- Added `encodeHubName()` function.
### Changed
- Updated firmware to v3.1.0c1.

## 4.13.0-beta.1 - 2021-09-21
### Changed
- Updated dependencies.
- Updated firmware to v3.1.0b1.

## 4.13.0-alpha.5 - 2021-08-30
### Changed
- Updated firmware to v3.1.0a4.

## 4.13.0-alpha.4 - 2021-08-13
### Added
- Added firmware version constant.

## 4.13.0-alpha.3 - 2021-07-19
### Changed
- Updated firmware to v3.1.0a3.

## 4.13.0-alpha.2 - 2021-07-06
### Changed
- Updated firmware to v3.1.0a2.

## 4.13.0-alpha.1 - 2021-06-23
### Changed
- Updated firmware to v3.1.0a1.

## 4.12.0 - 2021-06-08
### Changed
- Updated firmware to v3.0.0.

## 4.11.0 - 2021-05-11
### Changed
- Updated firmware to v3.0.0c1.

## 4.10.0 - 2021-04-26
### Changed
- Updated firmware to v3.0.0b6.

## 4.9.0 - 2021-04-12
### Changed
- Updated firmware to v3.0.0b5.

## 4.8.0 - 2021-04-05
### Changed
- Updated firmware to v3.0.0b4.

## 4.7.1 - 2021-04-02
### Changed
- Rerelease firmware to v3.0.0b3 due to bug.

## 4.7.0 - 2021-03-30
### Changed
- Updated firmware to v3.0.0b3.

## 4.6.0 - 2021-02-19
### Changed
- Updated firmware to v3.0.0b2.

## 4.5.0 - 2021-01-22
### Changed
- Updated firmware to v3.0.0b1.

## 4.4.0 - 2021-01-11
### Changed
- Publish to NPM instead of GitHub.

## 4.3.0 - 2021-01-08
### Changed
- Updated firmware to v3.0.0a13.

## 4.2.0 - 2020-12-11
### Changed
- Updated firmware to v3.0.0a12.

## 4.1.0 - 2020-11-20
### Added
- Allow `ArrayBuffer` argument in `FirmwareReader.load()`.

## 4.0.0 - 2020-11-20
### Added
- Added new `FirmwareMetadata` interface for strongly typing `firmware.metadata.json`.
- Added new `FirmwareReader` class for reading firmware.zip files.
### Changed
- Moved zip file names into `zipFileNameMap` and added `HubType` enum as lookup key.
- Updated firmware to v3.0.0a11.

## 3.1.0 - 2020-11-09
### Changed
- Updated firmware to v3.0.0a10.

## 3.0.0 - 2020-10-09
### Changed
- Renamed `cplushub` to `technichub`.
- Updated firmware to v3.0.0a9.

## 2.8.0 - 2020-09-04
### Changed
- Updated firmware to v3.0.0a8.

## 2.7.0 - 2020-08-22
### Changed
- Updated firmware to v3.0.0a7.

## 2.6.0 - 2020-08-05
### Changed
- Updated firmware to v3.0.0a6.

## 2.5.0 - 2020-07-15
### Changed
- Updated firmware to v3.0.0a5.

## 2.4.0 - 2020-07-02
### Added
- `cityhub` constant for City hub firmware zip file.

## 2.3.0 - 2020-07-02
### Changed
- Firmware is now downloaded from GitHub releases.
- Updated firmware to v3.0.0a4.

## 2.2.0 - 2020-06-07
### Changed
- Updated firmware to v3.0.0a3.

## 2.1.0 - 2020-06-04
### Changed
- Updated firmware to v3.0.0a2.

## 2.0.0 - 2020-06-04
### Changed
- Updated firmware to v3.0.0a1.

## 1.0.0 - 2020-05-025
### Added
- Firmware v3.0.0a0.
- Constants to get firmware zip file name.
