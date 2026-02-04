# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-02-04

### Changed
- **BREAKING**: Migrated from Arduino IDE to PlatformIO
- Completely refactored codebase into modular components
- Improved code formatting and documentation
- Enhanced error handling throughout

### Added
- `LedController` class for LED management
- `NfcReader` class for NFC operations
- Proper header/implementation file separation
- Comprehensive configuration via `Config.h`
- GitHub Actions CI/CD pipeline
- Full documentation (README, Hardware guide, API reference)
- MIT License
- `.gitignore` for proper version control

### Removed
- Legacy Arduino IDE `.ino` file structure
- Hardcoded credentials (now in separate config)

## [1.0.0] - Initial Release

### Features
- Basic NFC tag reading
- Spotify playback control
- WiFi Manager integration
- NeoPixel status LEDs
- Sound reactive mode
