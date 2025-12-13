# Heltec CubeCell MeshCore Repeater

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Heltec%20CubeCell-green.svg)](https://heltec.org/project/htcc-ab01/)

Production-ready LoRa mesh network repeater firmware for Heltec CubeCell devices. Forwards packets intelligently with SNR-based collision avoidance, supports multiple encrypted channels, and responds to network queries.

## Features

- **Complete Routing Support** - Full MeshCore protocol compatibility:
  - FLOOD routing with SNR-based adaptive delays (better signal = forward first)
  - DIRECT routing for point-to-point communication paths
  - Transport code support for network segmentation/bridging
- **Multi-Channel Support** - Monitor public channel + up to 8 private encrypted channels (AES-128)
- **Network Discovery** - Respond to discovery requests for network topology mapping
- **Network Commands** - Respond to `!status` (uptime/stats) and `!advert` (node announcement) on any channel
- **Path Tracing** - Handle TRACE packets for network diagnostics with SNR recording
- **All Payload Types** - Forward all MeshCore packet types (REQ, RESPONSE, ACK, PATH, MULTIPART, etc.)
- **Deduplication & Loop Prevention** - Hash-based packet cache prevents duplicates and routing loops
- **Power Optimized** - Light sleep mode between operations extends battery life
- **Debug Mode** - Optional serial logging for development and troubleshooting

## Hardware

- **Device**: Heltec CubeCell HTCC-AB02A
- **Radio**: Built-in SX1262 LoRa
- **Power**: 3.7V LiPo battery (optional)

See [docs/HARDWARE_HTCC-AB02A.md](docs/HARDWARE_HTCC-AB02A.md) for detailed specifications.

## Quick Start

### Prerequisites

- [PlatformIO](https://platformio.org/)
- USB cable for programming
- Compatible MeshCore network

### Installation

```bash
git clone https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater.git
cd Heltec-Cubcell-MeshCore-Repeater

# Production build (optimized, no logging)
pio run -e cubecell_board --target upload

# Debug build (with serial logging)
pio run -e cubecell_board_debug --target upload
pio device monitor
```

## Configuration

Edit `src/core/Config.h` to configure your repeater:

### Essential Settings

**LoRa Radio** - Must match your network and comply with local regulations:
- `FREQUENCY` - Default 869.618 MHz (EU ISM band)
- `SPREADING_FACTOR` - Default SF8
- `TX_POWER` - Default 21 dBm

**Node Identity**:
- `NODE_NAME` - Node name prefix (e.g., "VieZe Rogue")
- Custom node ID/hash (optional, defaults to hardware-generated)

**Private Channels** - Add your channel keys (hex format, 16 bytes):
```cpp
constexpr const char* PRIVATE_CHANNEL_KEYS[] = {
  "b4a28381f505eb67e696ed3d2294c81f",
  // Add more channels here
};
```

**Forwarding**:
- `ENABLED` - Enable/disable packet forwarding
- `MIN_RSSI_TO_FORWARD` - Minimum signal strength (-120 dBm default)
- Delay parameters for collision avoidance tuning

**Power Management**:
- `LIGHT_SLEEP_ENABLED` - Enable light sleep mode (true recommended)

## Usage

### Network Commands

Send these commands on public or private channels to query nearby repeaters:

**`!status`** - Get node status
- Response format: `NodeName XX: W:1h 5m S:23h 12m P:456`
- Shows wake time, sleep time, and packet count
- Rate limited to once per minute

**`!advert`** - Request node advertisement
- Nodes respond with ADVERT packet containing node type and name
- Useful for discovering nearby repeaters
- Rate limited to once per minute

### Monitoring (Debug Build)

Connect serial monitor at 115200 baud to view:
- Packet reception with RSSI/SNR
- Forwarding decisions and delays
- Processor pipeline execution
- Network statistics

```bash
pio device monitor
```

## How It Works

**Intelligent Routing**: 
- **FLOOD routing**: Nodes calculate forwarding delay based on signal quality (SNR). Better signal = shorter delay, so the best-positioned node forwards first. Others hear the transmission and cancel their pending forward, avoiding collisions.
- **DIRECT routing**: Packets follow a specified path hop-by-hop. Each node checks if it's the next hop, removes itself from the path, and forwards with minimal delay for fast delivery.
- **Transport codes**: Preserved during forwarding to enable network segmentation and bridging.

**Processing Pipeline**: Packets flow through priority-ordered processors:
1. Deduplicator (priority 10) - Filter duplicates
2. PacketForwarder (20) - Forward FLOOD/DIRECT packets with adaptive delays
3. TraceHandler (30) - Handle TRACE packets for path diagnostics
4. StatusResponder (35) - Process `!status` commands
5. AdvertResponder (35) - Process `!advert` commands
6. DiscoveryResponder (36) - Respond to network discovery requests
7. PacketLogger (99) - Debug logging

**Security**: Ed25519 identity generated on first boot from entropy (ADC + timing jitter). Private channels use AES-128 encryption. Keys stored in plaintext EEPROM.

**Power Management**: MCU sleeps when idle, wakes on radio interrupt. No packets missed.

## Protocol Support

- **Routing**: Full support for all MeshCore V1 routing modes
  - FLOOD - Multi-hop broadcast with path building
  - DIRECT - Point-to-point along specified path
  - TRANSPORT_FLOOD - Flood with transport codes for segmentation
  - TRANSPORT_DIRECT - Direct with transport codes
- **Payloads**: All 13 MeshCore V1 payload types
  - REQ, RESPONSE, TXT_MSG, ACK, ADVERT, GRP_TXT, GRP_DATA
  - ANON_REQ, PATH, TRACE, MULTIPART, CONTROL, RAW_CUSTOM
- **Channels**: 1 public + up to 8 private encrypted channels (AES-128)

## Troubleshooting

**No packets received**: Check LoRa frequency/SF match your network. Verify antenna connection.

**Not forwarding**: Ensure `Forwarding::ENABLED = true` in Config.h. Check `MIN_RSSI_TO_FORWARD` threshold.

**High power consumption**: Use production build (not debug). Enable light sleep mode.

For issues, see [GitHub Issues](https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater/issues).

## Regulatory & Security

**Radio Compliance**: You are responsible for complying with local radio regulations. Default is 869.618 MHz @ 21 dBm (EU ISM band). Duty cycle enforcement is NOT implemented - manual compliance required (EU: ≤1% on 868 MHz).

**Cryptographic Keys**: Keys generated from limited entropy sources (suitable for device ID, not high-security). Physical access to device allows key extraction (EEPROM storage).

## License

MIT License - see [LICENSE](LICENSE) file.

## Acknowledgments

Built for [Heltec CubeCell HTCC-AB02A](https://docs.heltec.cn/en/node/asr650x/htcc_ab02a/). Compatible with MeshCore protocol.
