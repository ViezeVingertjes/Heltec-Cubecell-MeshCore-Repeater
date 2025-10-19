# Heltec CubeCell MeshCore Repeater

A LoRa mesh network repeater for Heltec CubeCell devices. Receives packets and forwards them with adaptive delays to reduce collisions.

## Features

- **Packet Forwarding**: Forwards received packets with SNR-based delays
- **Deduplication**: Prevents forwarding the same packet twice
- **Path Tracking**: Avoids routing loops
- **Statistics**: Tracks packets and network activity
- **Configurable**: Adjust RSSI thresholds, delays, and airtime limits

## Quick Start

### Installation

Requires [PlatformIO](https://platformio.org/):

```bash
git clone https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater.git
cd Heltec-Cubcell-MeshCore-Repeater
pio run --target upload
pio device monitor
```

### Configuration

Edit `src/core/Config.h` for key settings:

**LoRa Radio** (must match your network):
- `FREQUENCY`: 869618000 (869.618 MHz - adjust for your region)
- `SPREADING_FACTOR`: 8
- `TX_POWER`: 21 dBm

**Forwarding**:
- `ENABLED`: true (enable/disable forwarding)
- `MIN_RSSI_TO_FORWARD`: -120 (signal strength threshold)
- `RX_DELAY_BASE`: 2.5 (delay multiplier)

**⚠️ Important**: Check frequency and power comply with local regulations.

## Build Targets

- `cubecell_board`: Production build
- `cubecell_board_debug`: Debug build with logging

```bash
pio run -e cubecell_board_debug
```

## How It Works

Packets flow through a processing pipeline:
1. **LoRaReceiver** → receives packets
2. **Deduplicator** → filters duplicates
3. **PacketLogger** → logs details (debug mode)
4. **PacketStats** → tracks metrics
5. **TraceHandler** → manages traces
6. **PacketForwarder** → forwards with calculated delays

Better signal quality = shorter delays, so nodes with better reception forward first.

## Serial Monitor

Connect at 115200 baud to see:
- Node ID and configuration
- Received packets (RSSI, SNR, payload)
- Forwarding decisions
- Network statistics