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

- `cubecell_board`: **Production build** (speed-optimized, no logging)
- `cubecell_board_debug`: **Debug build** (logging enabled, lighter optimization)

```bash
# Production build (fastest)
pio run -e cubecell_board --target upload

# Debug build (with logging)
pio run -e cubecell_board_debug --target upload
pio device monitor
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

## Performance Optimizations

The repeater is optimized for **low latency** and **fast execution**:

**Code-level optimizations:**
- **Non-blocking processing**: No artificial delays in packet pipeline
- **Batch queue processing**: All ready packets transmitted in single loop iteration
- **Immediate forwarding**: High-SNR packets forwarded with <20ms delay
- **Optimized TRACE handling**: DIRECT-routed traces transmit immediately without collision delays

**Compiler optimizations** (production build):
- **-O2**: Optimize for speed over size
- **-flto**: Link Time Optimization for better cross-module optimization
- **-ffast-math**: Faster floating-point math for SNR calculations
- **-funroll-loops**: Loop unrolling for performance-critical paths
- **-finline-functions**: Aggressive inlining to reduce call overhead

**Expected latency improvements:**
- TRACE packets: ~150-275ms faster per hop
- High-SNR forwarding: ~50ms faster per hop
- Batch processing: Eliminates queuing delays

## Serial Monitor

Connect at 115200 baud to see:
- Node ID and configuration
- Received packets (RSSI, SNR, payload)
- Forwarding decisions
- Network statistics