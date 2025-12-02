# Heltec CubeCell MeshCore Repeater

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](CHANGELOG.md)
[![Platform](https://img.shields.io/badge/platform-Heltec%20CubeCell-green.svg)](https://heltec.org/project/htcc-ab01/)

A production-ready LoRa mesh network repeater firmware for Heltec CubeCell devices. This firmware enables devices to act as intelligent repeaters that forward packets with adaptive collision avoidance and power management.

## Features

- **Intelligent Packet Forwarding**: SNR-based adaptive delays to minimize collisions
- **Deduplication**: Prevents forwarding duplicate packets with configurable cache
- **Loop Prevention**: Path tracking to avoid routing loops
- **Ping Responder**: Responds to `!ping` commands with battery and node status
- **Power Management**: Light sleep mode between operations for extended battery life
- **Production Optimized**: Compiler optimizations for low-latency packet processing
- **Debug Mode**: Optional logging for development and troubleshooting

## Hardware Requirements

- **Device**: Heltec CubeCell (tested on CubeCell Board)
- **LoRa**: Built-in SX1262 radio
- **Power**: 3.7V LiPo battery (optional, for portable operation)

## Prerequisites

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE with CubeCell support
- USB cable for programming
- Compatible LoRa network running MeshCore protocol

## Installation

### Using PlatformIO (Recommended)

```bash
git clone https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater.git
cd Heltec-Cubcell-MeshCore-Repeater

# Install dependencies (if needed)
pio lib install

# Build and upload
pio run --target upload
```

### Build Targets

- **`cubecell_board`**: Production build with optimizations (-O2, LTO) and logging disabled
- **`cubecell_board_debug`**: Debug build with logging enabled

```bash
# Production build (recommended for deployment)
pio run -e cubecell_board --target upload

# Debug build (for development)
pio run -e cubecell_board_debug --target upload
pio device monitor
```

## Configuration

Edit `src/core/Config.h` to configure the repeater:

### LoRa Radio Settings

**⚠️ Important**: These settings must match your network. Ensure compliance with local regulations.

```cpp
namespace LoRa {
  constexpr uint32_t FREQUENCY = 869618000;    // 869.618 MHz (adjust for your region)
  constexpr uint8_t SPREADING_FACTOR = 8;       // SF8
  constexpr uint8_t TX_POWER = 21;              // 21 dBm (check local regulations)
}
```

### Forwarding Configuration

```cpp
namespace Forwarding {
  constexpr bool ENABLED = true;                 // Enable/disable forwarding
  constexpr int16_t MIN_RSSI_TO_FORWARD = -120; // Minimum signal strength
  constexpr float RX_DELAY_BASE = 2.5f;         // Base delay multiplier
  constexpr float TX_DELAY_FACTOR = 2.0f;       // Jitter slot multiplier
}
```

### Power Management

```cpp
namespace Power {
  constexpr bool LIGHT_SLEEP_ENABLED = true;     // Enable light sleep mode
  constexpr bool DYNAMIC_VEXT_CONTROL = true;    // Dynamic LED power control
}
```

## How It Works

### Packet Processing Pipeline

Received packets flow through a priority-ordered processing chain:

1. **LoRaReceiver** - Receives packets from the radio
2. **Deduplicator** (priority 10) - Filters duplicate packets
3. **PacketForwarder** (priority 20) - Forwards packets with calculated delays
4. **TraceHandler** (priority 30) - Handles TRACE packets
5. **PacketLogger** (priority 40) - Logs packet details (debug mode only)
6. **PingResponder** (priority 50) - Responds to `!ping` commands

### Adaptive Forwarding

The repeater calculates forwarding delays based on signal quality (SNR):
- **Better signal** (higher SNR) → **shorter delay** → forwards first
- **Weaker signal** (lower SNR) → **longer delay** → waits for better nodes

This ensures nodes with the best reception forward packets first, optimizing network efficiency.

### Power Optimization

When idle, the repeater enters light sleep mode to conserve battery:
- MCU sleeps between operations
- Wakes on radio interrupt (no packets missed)
- Dynamic Vext control for LED power
- Serial output disabled in production builds

## Usage

### Monitoring

Connect a serial monitor at 115200 baud (debug build only):

```bash
pio device monitor
```

Output includes:
- Node ID and configuration on startup
- Received packets with RSSI/SNR
- Forwarding decisions and delays
- Battery status and network statistics

### Ping Command

Send `!ping` message to the public channel to query nearby repeaters. They will respond with:
- Node hash (last byte of Node ID)
- Battery voltage and percentage
- Estimated hours remaining

**Rate limiting**: Repeaters respond at most once per minute to prevent spam.

## Architecture

### Directory Structure

```
src/
├── core/           # Core utilities (config, logging, crypto, time sync)
├── crypto/         # Cryptographic functions (AES, SHA256, Ed25519)
├── mesh/           # Mesh networking (dispatcher, queue, processors)
├── power/          # Power management and battery monitoring
└── radio/          # LoRa radio (receiver, transmitter)
```

### Key Components

- **PacketDispatcher**: Priority-based packet processor chain
- **PacketQueue**: Time-sorted queue for delayed transmissions
- **CryptoIdentity**: Ed25519 identity management
- **BatteryMonitor**: Voltage monitoring and estimation
- **PowerManager**: Sleep mode coordination

## Development

### Adding Custom Processors

Implement the `IPacketProcessor` interface:

```cpp
class MyProcessor : public MeshCore::IPacketProcessor {
public:
  MeshCore::ProcessResult processPacket(
    const MeshCore::PacketEvent &event,
    MeshCore::ProcessingContext &ctx) override;
  
  const char *getName() const override { return "MyProcessor"; }
  uint8_t getPriority() const override { return 25; }
};
```

Register in `main.cpp`:

```cpp
static MyProcessor myProcessor;
dispatcher.addProcessor(&myProcessor);
```

### Compiler Optimizations

Production builds include aggressive optimizations:
- `-O2`: Speed optimization
- `-flto`: Link-time optimization
- `-ffast-math`: Fast floating-point operations
- `-funroll-loops`: Loop unrolling
- `-finline-functions`: Aggressive inlining

## Troubleshooting

### No packets received
- Check LoRa frequency matches your network
- Verify spreading factor and bandwidth settings
- Ensure antenna is connected properly

### High power consumption
- Enable light sleep mode in `Config.h`
- Disable logging in production builds
- Check for continuous LED operation

### Packets not forwarding
- Verify `Config::Forwarding::ENABLED = true`
- Check `MIN_RSSI_TO_FORWARD` threshold
- Review signal quality (RSSI/SNR) in debug logs

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with clear commit messages
4. Test thoroughly with both build targets
5. Submit a pull request

## Security Considerations

### Cryptographic Key Generation

**⚠️ Important Security Notice**

The firmware generates Ed25519 cryptographic keys on first boot using entropy collected from:
- Analog noise (ADC readings)
- Timing jitter (micros/millis)
- Arduino pseudo-random number generator

This entropy source is **NOT cryptographically secure** and may be insufficient for high-security applications. The generated keys are suitable for device identification but may be predictable in some circumstances.

**Recommendations for production deployments:**
1. Generate keys in a high-entropy environment (PC with hardware RNG)
2. Load pre-generated keys onto devices
3. Consider hardware RNG if your platform supports it
4. Understand that physical access to device = key compromise (keys stored in plaintext EEPROM)

### Private Key Storage

Private keys are stored in plaintext in the device's EEPROM. This is acceptable for mesh network device identification but means:
- **Physical access** to the device allows key extraction
- Keys persist across firmware updates
- No remote key compromise possible (keys never transmitted)

### Regulatory Compliance

**⚠️ Radio Regulations Warning**

Users are responsible for ensuring compliance with local radio regulations:
- **Frequency**: Default 869.618 MHz (ISM band) - verify legality in your region
- **Transmit Power**: Default 21 dBm - check maximum allowed in your region
- **Duty Cycle**: Currently **NOT enforced** by firmware - you must track this externally
  - EU 868 MHz band requires ≤1% duty cycle
  - Other regions have different requirements

Configure `Config.h` appropriately for your region and use case.

## License

MIT License - see [LICENSE](LICENSE) file for details.

Copyright (c) 2025 Heltec CubeCell MeshCore Repeater Contributors

## Acknowledgments

- Built on the [Heltec CubeCell platform](https://heltec.org/project/htcc-ab01/)
- Uses Ed25519 for cryptographic identity
- Compatible with MeshCore protocol

## Support

For issues, questions, or contributions, please use the [GitHub issue tracker](https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater/issues).
