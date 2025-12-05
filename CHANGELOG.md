# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-12-02

### Added
- Initial production release
- Intelligent packet forwarding with SNR-based adaptive delays
- Deduplication with circular buffer cache
- Path tracking for loop prevention
- Ping responder with rate limiting (1 minute)
- Battery monitoring and status reporting
- Power management with light sleep mode
- Public channel announcer for mesh communication
- Comprehensive error handling with Result<T> types
- Packet validation and memory safety checks
- Production and debug build configurations
- Ed25519 cryptographic identity management
- AES-128 encryption for channel messages

### Security
- Note: Entropy collection for key generation uses weak sources (analogRead, millis, micros)
- Private keys stored in plaintext EEPROM (physical security required)
- Public channel PSK is hardcoded by design

### Configuration
- LoRa frequency: 869.618 MHz (configurable)
- Spreading Factor: SF8
- Transmit power: 21 dBm (configurable)
- Ping response rate limit: 1 minute
- Deduplication cache: 60 seconds

### Known Limitations
- Duty cycle enforcement is disabled (user must ensure regulatory compliance)
- Key generation entropy may be insufficient for high-security applications
- No over-the-air (OTA) update capability

## [Unreleased]

### Planned
- Duty cycle tracking and enforcement
- Hardware RNG support for improved entropy
- OTA update capability
- Firmware version reporting over the air

